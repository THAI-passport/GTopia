package main

import (
	"strings"
	"time"

	"github.com/bwmarrin/discordgo"
)

type Reply struct {
	Text  string
	Embed *discordgo.MessageEmbed
}

type Option struct {
	Name     string
	Desc     string
	Required bool
	Default  string
	Choices  []string
}

type Command struct {
	Name     string
	Desc     string
	Rank     int
	Prefix   bool
	Slash    bool
	Local    bool
	Cooldown time.Duration
	Options  []Option
}

var punishOptions = []Option{
	{Name: "userid", Desc: "Player user ID", Required: true},
	{Name: "minutes", Desc: "Duration in minutes", Required: true},
	{Name: "type", Desc: "Punish type", Default: "soft", Choices: []string{"soft", "hard", "ip", "device"}},
	{Name: "silent", Desc: "Silent punishment", Default: "false", Choices: []string{"false", "true"}},
	{Name: "reason", Desc: "Reason"},
}

var commandList = []Command{
	{Name: "help", Desc: "List the commands you can use", Rank: 0, Prefix: true, Slash: true, Local: true},
	{Name: "mute", Desc: "Duct-tape a player's mouth", Rank: 2, Prefix: true, Slash: false, Cooldown: 2 * time.Second, Options: punishOptions},
	{Name: "curse", Desc: "Curse a player to netherworld", Rank: 2, Prefix: true, Slash: false, Cooldown: 2 * time.Second, Options: punishOptions},
	{Name: "pban", Desc: "Ban a player", Rank: 3, Prefix: true, Slash: false, Cooldown: 3 * time.Second, Options: punishOptions},
	{Name: "setplaymod", Desc: "Add playmod to player", Rank: 3, Prefix: true, Slash: false, Cooldown: 2 * time.Second, Options: []Option{
		{Name: "userid", Desc: "Player user ID", Required: true},
		{Name: "minutes", Desc: "Duration in minutes", Required: true},
		{Name: "playmodid", Desc: "Playmod ID", Required: true},
	}},
	{Name: "holiday", Desc: "Manage holidays (run without args for subcommands)", Rank: 3, Prefix: true, Slash: false, Cooldown: time.Second, Options: []Option{
		{Name: "subcommand", Desc: "list, info, start, end, create, ...", Required: true},
		{Name: "args", Desc: "Subcommand arguments"},
	}},
	{Name: "setrole", Desc: "Update player's role", Rank: 4, Prefix: true, Slash: false, Cooldown: 5 * time.Second, Options: []Option{
		{Name: "userid", Desc: "Player user ID", Required: true},
		{Name: "roleid", Desc: "Role ID", Required: true},
	}},
}

func findCommand(name string, wantSlash bool) *Command {
	name = strings.ToLower(name)
	for index := range commandList {
		cmd := &commandList[index]
		if cmd.Name != name {
			continue
		}
		if wantSlash && cmd.Slash {
			return cmd
		}
		if !wantSlash && cmd.Prefix {
			return cmd
		}
	}
	return nil
}

func usageText(prefix string, cmd *Command) string {
	text := prefix + cmd.Name
	for _, option := range cmd.Options {
		if option.Required {
			text += " <" + option.Name + ">"
		} else {
			text += " [" + option.Name + "]"
		}
	}
	return text
}

func helpText(prefix string, rank int) string {
	text := "=== Available Commands ===\n"
	for index := range commandList {
		cmd := &commandList[index]
		if cmd.Rank > rank {
			continue
		}
		text += usageText(prefix, cmd) + " - " + cmd.Desc + "\n"
	}
	return text
}

func prefixLine(cmd *Command, fields []string) string {
	return strings.TrimSpace(cmd.Name + " " + strings.Join(fields[1:], " "))
}

func slashLine(cmd *Command, options []*discordgo.ApplicationCommandInteractionDataOption) string {
	given := make(map[string]string)
	for _, option := range options {
		given[option.Name] = strings.TrimSpace(option.StringValue())
	}

	line := cmd.Name
	for _, option := range cmd.Options {
		value := given[option.Name]
		if value == "" {
			value = option.Default
		}
		line += " " + value
	}
	return strings.TrimSpace(line)
}

func slashDefinitions() []*discordgo.ApplicationCommand {
	var definitions []*discordgo.ApplicationCommand
	for _, cmd := range commandList {
		if !cmd.Slash {
			continue
		}
		definition := &discordgo.ApplicationCommand{Name: cmd.Name, Description: cmd.Desc}
		for _, option := range cmd.Options {
			slashOption := &discordgo.ApplicationCommandOption{
				Type:        discordgo.ApplicationCommandOptionString,
				Name:        option.Name,
				Description: option.Desc,
				Required:    option.Required,
			}
			for _, choice := range option.Choices {
				slashOption.Choices = append(slashOption.Choices, &discordgo.ApplicationCommandOptionChoice{Name: choice, Value: choice})
			}
			definition.Options = append(definition.Options, slashOption)
		}
		definitions = append(definitions, definition)
	}
	return definitions
}
