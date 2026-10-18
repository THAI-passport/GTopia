package main

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/bwmarrin/discordgo"
)

type Config struct {
	Token                string              `json:"token"`
	Prefix               string              `json:"prefix"`
	GuildID              string              `json:"guild_id"`
	AdminAddr            string              `json:"admin_addr"`
	Account              string              `json:"account"`
	Password             string              `json:"password"`
	Ranks                map[string][]string `json:"ranks"`
	OnlineStatusInterval int                 `json:"online_status_interval"`
}

func LoadConfig(path string) (*Config, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}

	var config Config
	err = json.Unmarshal(data, &config)
	if err != nil {
		return nil, err
	}

	if config.Token == "" || config.AdminAddr == "" || config.Account == "" || config.Password == "" {
		return nil, fmt.Errorf("token, admin_addr, account and password are required")
	}
	if config.Prefix == "" {
		config.Prefix = "!"
	}
	return &config, nil
}

func (config *Config) GetRank(userID string) int {
	best := 0
	for rankText, ids := range config.Ranks {
		rank, err := strconv.Atoi(rankText)
		if err != nil {
			continue
		}
		for _, id := range ids {
			if id == userID && rank > best {
				best = rank
			}
		}
	}
	return best
}

type Bot struct {
	config        *Config
	admin         *AdminClient
	lock          sync.Mutex
	lastUsed      map[string]time.Time
	statusStarted bool
}

func main() {
	path := "config.json"
	if len(os.Args) > 1 {
		path = os.Args[1]
	}

	config, err := LoadConfig(path)
	if err != nil {
		log.Fatal("config error: ", err)
	}

	bot := &Bot{config: config, admin: NewAdminClient(config), lastUsed: make(map[string]time.Time)}
	go bot.admin.Run()

	session, err := discordgo.New("Bot " + config.Token)
	if err != nil {
		log.Fatal(err)
	}
	session.Identify.Intents = discordgo.IntentsGuildMessages | discordgo.IntentsMessageContent

	session.AddHandler(bot.onReady)
	session.AddHandler(bot.onMessage)
	session.AddHandler(bot.onInteraction)

	err = session.Open()
	if err != nil {
		log.Fatal("discord error: ", err)
	}
	defer session.Close()
	log.Println("bot running, press Ctrl+C to stop")

	stop := make(chan os.Signal, 1)
	signal.Notify(stop, os.Interrupt)
	<-stop
	bot.admin.Close()
}

func (bot *Bot) onReady(session *discordgo.Session, ready *discordgo.Ready) {
	if !bot.statusStarted {
		bot.statusStarted = true
		go bot.statusLoop(session)
	}

	_, err := session.ApplicationCommandBulkOverwrite(ready.User.ID, bot.config.GuildID, slashDefinitions())
	if err != nil {
		log.Println("slash registration failed:", err)
	}

	log.Println("received ready from bot", ready.User.Username, ready.User.ID)
}

func (bot *Bot) statusLoop(session *discordgo.Session) {
	for {
		status := discordgo.UpdateStatusData{Status: "online"}

		if !bot.admin.Connected() {
			status.Status = "dnd"
			status.Activities = []*discordgo.Activity{{Name: "server offline", Type: discordgo.ActivityTypeWatching}}
		} else {
			reply := bot.admin.Execute("onlinestats")
			count, err := strconv.Atoi(strings.TrimSpace(reply.Text))
			if err == nil {
				name := fmt.Sprintf("with %d players", count)
				status.Activities = []*discordgo.Activity{{Name: name, Type: discordgo.ActivityTypeGame}}
			}
		}

		err := session.UpdateStatusComplex(status)
		if err != nil {
			log.Println("status update failed:", err)
		}
		time.Sleep(time.Duration(bot.config.OnlineStatusInterval) * time.Second)
	}
}

func (bot *Bot) onCooldown(userID string, cmd *Command) bool {
	if cmd.Cooldown == 0 {
		return false
	}

	bot.lock.Lock()
	defer bot.lock.Unlock()

	key := userID + "|" + cmd.Name
	last, found := bot.lastUsed[key]
	if found && time.Since(last) < cmd.Cooldown {
		return true
	}
	bot.lastUsed[key] = time.Now()
	return false
}

func (bot *Bot) runCommand(cmd *Command, rank int, line string) Reply {
	if cmd.Name == "help" {
		return Reply{Text: helpText(bot.config.Prefix, rank)}
	}
	return bot.admin.Execute(line)
}

func (bot *Bot) onMessage(session *discordgo.Session, message *discordgo.MessageCreate) {
	if message.Author == nil || message.Author.Bot {
		return
	}
	if !strings.HasPrefix(message.Content, bot.config.Prefix) {
		return
	}

	fields := strings.Fields(message.Content[len(bot.config.Prefix):])
	if len(fields) == 0 {
		return
	}

	cmd := findCommand(fields[0], false)
	if cmd == nil {
		return
	}

	rank := bot.config.GetRank(message.Author.ID)
	if rank < cmd.Rank {
		return
	}

	if !cmd.Local && !bot.admin.Connected() {
		session.ChannelMessageSendReply(message.ChannelID, "The game server is offline right now, try again in a moment.", message.Reference())
		return
	}

	if bot.onCooldown(message.Author.ID, cmd) {
		session.ChannelMessageSendReply(message.ChannelID, "Slow down, try again in a moment.", message.Reference())
		return
	}

	line := prefixLine(cmd, fields)
	log.Println("prefix command from", message.Author.Username, "(", message.Author.ID, "):", line)

	var waitingMessage *discordgo.Message
	if !cmd.Local {
		waitingMessage, _ = session.ChannelMessageSendReply(message.ChannelID, "Sending to the server...", message.Reference())
	}

	reply := bot.runCommand(cmd, rank, line)

	if waitingMessage != nil {
		edit := discordgo.NewMessageEdit(message.ChannelID, waitingMessage.ID)
		if reply.Embed != nil {
			embeds := []*discordgo.MessageEmbed{reply.Embed}
			empty := ""
			edit.Content = &empty
			edit.Embeds = &embeds
		} else {
			content := formatText(reply.Text)
			edit.Content = &content
		}
		_, err := session.ChannelMessageEditComplex(edit)
		if err != nil {
			log.Println("edit failed:", err)
		}
		return
	}

	send := &discordgo.MessageSend{Reference: message.Reference()}
	if reply.Embed != nil {
		send.Embeds = []*discordgo.MessageEmbed{reply.Embed}
	} else {
		send.Content = formatText(reply.Text)
	}
	_, err := session.ChannelMessageSendComplex(message.ChannelID, send)
	if err != nil {
		log.Println("send failed:", err)
	}
}

func (bot *Bot) onInteraction(session *discordgo.Session, interaction *discordgo.InteractionCreate) {
	if interaction.Type != discordgo.InteractionApplicationCommand {
		return
	}

	data := interaction.ApplicationCommandData()
	cmd := findCommand(data.Name, true)
	if cmd == nil {
		return
	}

	user := interaction.User
	if interaction.Member != nil {
		user = interaction.Member.User
	}
	if user == nil {
		return
	}

	rank := bot.config.GetRank(user.ID)
	if rank < cmd.Rank {
		if cmd.Local {
			replyEphemeral(session, interaction, "You don't have permission to use this command.")
		}
		return
	}

	if !cmd.Local && !bot.admin.Connected() {
		replyEphemeral(session, interaction, "The game server is offline right now, try again in a moment.")
		return
	}

	if bot.onCooldown(user.ID, cmd) {
		replyEphemeral(session, interaction, "Slow down, try again in a moment.")
		return
	}

	err := session.InteractionRespond(interaction.Interaction, &discordgo.InteractionResponse{
		Type: discordgo.InteractionResponseDeferredChannelMessageWithSource,
	})
	if err != nil {
		log.Println("defer failed:", err)
		return
	}

	line := slashLine(cmd, data.Options)
	log.Println("slash command from", user.Username, "(", user.ID, "):", line)

	reply := bot.runCommand(cmd, rank, line)

	edit := &discordgo.WebhookEdit{}
	if reply.Embed != nil {
		embeds := []*discordgo.MessageEmbed{reply.Embed}
		edit.Embeds = &embeds
	} else {
		content := formatText(reply.Text)
		edit.Content = &content
	}

	_, err = session.InteractionResponseEdit(interaction.Interaction, edit)
	if err != nil {
		log.Println("edit failed:", err)
	}
}

func replyEphemeral(session *discordgo.Session, interaction *discordgo.InteractionCreate, text string) {
	session.InteractionRespond(interaction.Interaction, &discordgo.InteractionResponse{
		Type: discordgo.InteractionResponseChannelMessageWithSource,
		Data: &discordgo.InteractionResponseData{Content: text, Flags: discordgo.MessageFlagsEphemeral},
	})
}

func formatText(text string) string {
	text = strings.TrimSpace(text)
	if text == "" {
		text = "(empty response)"
	}
	if len(text) > 1900 {
		text = text[:1900] + "\n..."
	}
	return "```\n" + text + "\n```"
}
