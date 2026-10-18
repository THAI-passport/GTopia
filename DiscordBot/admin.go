package main

import (
	"encoding/binary"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/bwmarrin/discordgo"
)

type AdminClient struct {
	config  *Config
	lock    sync.Mutex
	conn    net.Conn
	pending map[uint32]chan Reply
	nextID  uint32
}

func NewAdminClient(config *Config) *AdminClient {
	return &AdminClient{config: config, pending: make(map[uint32]chan Reply)}
}

func (client *AdminClient) Run() {
	for {
		err := client.connect()
		log.Println("admin disconnected:", err)
		time.Sleep(3 * time.Second)
	}
}

func (client *AdminClient) Close() {
	client.lock.Lock()
	defer client.lock.Unlock()
	if client.conn != nil {
		client.conn.Close()
	}
}

func (client *AdminClient) connect() error {
	conn, err := net.DialTimeout("tcp", client.config.AdminAddr, 10*time.Second)
	if err != nil {
		return err
	}
	defer conn.Close()

	authText := "auth " + client.config.Account + " " + client.config.Password
	time.Sleep(500 * time.Millisecond)
	err = writeFrame(conn, authText)
	if err != nil {
		return err
	}

	conn.SetReadDeadline(time.Now().Add(10 * time.Second))
	answer, err := readFrame(conn)
	if err != nil {
		log.Println("no auth answer from server:", err)
		return err
	}
	log.Printf("auth answer: %q", answer)
	if answer != "OK" {
		return fmt.Errorf("auth rejected: %s", answer)
	}
	conn.SetReadDeadline(time.Time{})

	client.lock.Lock()
	client.conn = conn
	client.lock.Unlock()
	log.Println("admin connected")

	stop := make(chan bool)
	go client.keepAlive(stop)

	for {
		payload, err := readFrame(conn)
		if err != nil {
			close(stop)
			client.lock.Lock()
			client.conn = nil
			for id, channel := range client.pending {
				channel <- Reply{Text: "Lost connection to the admin server."}
				delete(client.pending, id)
			}
			client.lock.Unlock()
			return err
		}
		client.handlePacket(payload)
	}
}

func (client *AdminClient) keepAlive(stop chan bool) {
	for {
		select {
		case <-stop:
			return
		case <-time.After(5 * time.Minute):
			client.send("")
		}
	}
}

func (client *AdminClient) Connected() bool {
	client.lock.Lock()
	defer client.lock.Unlock()
	return client.conn != nil
}

func (client *AdminClient) handlePacket(payload string) {
	parts := strings.SplitN(payload, "|", 3)
	if len(parts) != 3 || (parts[0] != "RESP" && parts[0] != "RESPE") {
		log.Println("bad packet:", payload)
		return
	}

	id, err := strconv.ParseUint(parts[1], 10, 32)
	if err != nil || id == 0 {
		log.Println("server error:", parts[2])
		return
	}

	reply := Reply{Text: parts[2]}
	if parts[0] == "RESPE" {
		embed, err := parseEmbed(parts[2])
		if err != nil {
			reply = Reply{Text: "Server sent an invalid embed."}
		} else {
			reply = Reply{Embed: embed}
		}
	}

	client.lock.Lock()
	channel := client.pending[uint32(id)]
	client.lock.Unlock()

	if channel != nil {
		select {
		case channel <- reply:
		default:
		}
	}
}

func (client *AdminClient) Execute(line string) Reply {
	line = strings.ReplaceAll(line, "\n", " ")
	line = strings.ReplaceAll(line, "\r", " ")

	client.lock.Lock()
	client.nextID++
	id := client.nextID
	channel := make(chan Reply, 1)
	client.pending[id] = channel
	client.lock.Unlock()

	defer func() {
		client.lock.Lock()
		delete(client.pending, id)
		client.lock.Unlock()
	}()

	err := client.send(fmt.Sprintf("%d|%s", id, line))
	if err != nil {
		return Reply{Text: "Admin server is not reachable right now."}
	}

	select {
	case reply := <-channel:
		return reply
	case <-time.After(10 * time.Second):
		return Reply{Text: "Timed out waiting for the admin server."}
	}
}

func (client *AdminClient) send(payload string) error {
	client.lock.Lock()
	defer client.lock.Unlock()

	if client.conn == nil {
		return fmt.Errorf("not connected")
	}
	client.conn.SetWriteDeadline(time.Now().Add(5 * time.Second))
	return writeFrame(client.conn, payload)
}

func writeFrame(writer io.Writer, payload string) error {
	data := make([]byte, 4+len(payload))
	binary.LittleEndian.PutUint32(data, uint32(len(data)))
	copy(data[4:], payload)
	_, err := writer.Write(data)
	return err
}

func readFrame(reader io.Reader) (string, error) {
	header := make([]byte, 4)
	_, err := io.ReadFull(reader, header)
	if err != nil {
		return "", err
	}

	size := binary.LittleEndian.Uint32(header)
	if size < 4 || size > 4*1024*1024 {
		return "", fmt.Errorf("bad frame size %d", size)
	}

	body := make([]byte, size-4)
	_, err = io.ReadFull(reader, body)
	if err != nil {
		return "", err
	}
	return string(body), nil
}

func parseEmbed(body string) (*discordgo.MessageEmbed, error) {
	var embed discordgo.MessageEmbed
	err := json.Unmarshal([]byte(body), &embed)
	if err != nil {
		return nil, err
	}

	var fields []*discordgo.MessageEmbedField
	for _, field := range embed.Fields {
		if field.Name != "" && field.Value != "" {
			fields = append(fields, field)
		}
	}
	embed.Fields = fields
	return &embed, nil
}
