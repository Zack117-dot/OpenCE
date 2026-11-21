/* The real ai_conversation_line_begin (ai_communication.c) over one
conversation: a line that names no participant (a50's prison_kickin_ass,
the corporal's line) is spoken by the conversation's one participant's
named unit while it lives, else unspatialized; a line with no dialogue is
skipped; a line of a participant found is begun as before, and one of a
participant not found is skipped as before. */
#include "harness.h"

#include "config.inc"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define match_assert(file, line, condition) ((void)0)
#define error(...) ((void)0)
#define TAG_BLOCK_GET_ELEMENT(block, index, type) ((type *)(block)->address + (index))

#define SPEAKER_UNIT 0x10001L
#define OTHER_UNIT 0x10002L
#define ACTOR 0x20000L
#define SPEAKER_NAME 5

struct scenario { struct tag_block ai_conversations; };
struct actor_datum { struct { long unit_index; } meta; };
struct unit_datum { struct { long damage_flags; } object; };

static struct scenario scenario;
static struct ai_conversation definition;
static struct conversation_datum conversation;
static struct scenario_conversation_participant_view participants[2];
static struct scenario_conversation_line_view line;
static struct actor_datum actor = { { OTHER_UNIT } };
static struct unit_datum speaker = { { 0 } };

static struct conversation_datum *ai_conversation_driver_get(long index) { return &conversation; }
static struct scenario *global_scenario_get(void) { return &scenario; }
static struct actor_datum *actor_get(long index) { return &actor; }
static long object_index_from_name_index(short name_index) { return name_index == SPEAKER_NAME ? SPEAKER_UNIT : NONE; }
static struct unit_datum *unit_try_and_get(long index) { return index == SPEAKER_UNIT ? &speaker : NULL; }

#include "under_test.inc"

/* a conversation of count participants, the first naming the speaker's
unit, and its one line, of the participant given, whose first dialogue is
the sound given */
static void conversation_of(short count, short line_participant, long sound)
{
	short index;

	memset(&conversation, 0, sizeof(conversation));
	memset(participants, 0, sizeof(participants));
	memset(&line, 0, sizeof(line));
	for (index = 0; index < 2; index++)
	{
		participants[index].preexisting_object_name_index = index == 0 ? SPEAKER_NAME : NONE;
		participants[index].new_attach_object_name_index = NONE;
	}
	for (index = 0; index < MAXIMUM_DIALOGUE_VARIANTS_PER_CONVERSATION_PARTICIPANT; index++)
		line.dialogue[index].index = NONE;
	line.dialogue[0].index = sound;
	line.participant_index = line_participant;
	line.address_type = NONE;
	definition.participants.count = count;
	definition.participants.address = participants;
	definition.lines.count = 1;
	definition.lines.address = &line;
	scenario.ai_conversations.count = 1;
	scenario.ai_conversations.address = &definition;
	conversation.line_unit_index = 12345;
	speaker.object.damage_flags = 0;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	CASE("unnamed line")
	{
		conversation_of(1, NONE, 77);
		CHECK(ai_conversation_line_begin(0), "the line is not begun");
		CHECK(conversation.line_unit_index == SPEAKER_UNIT && !conversation.line_unspatialized,
			"spoken by %lx (unspatialized %d), not the participant's unit", conversation.line_unit_index,
			conversation.line_unspatialized);
		CHECK(conversation.line_sound_index == 77 && conversation.line_actor_index == NONE &&
			conversation.line_participant_index == NONE, "the line's sound or speaker wrong");
		return 0;
	}
	CASE("dead participant")
	{
		conversation_of(1, NONE, 77);
		speaker.object.damage_flags = 1L << _object_dead_bit;
		CHECK(ai_conversation_line_begin(0), "the line is not begun");
		CHECK(conversation.line_unit_index == NONE && conversation.line_unspatialized,
			"a dead unit speaks it (%lx)", conversation.line_unit_index);
		return 0;
	}
	CASE("two participants")
	{
		conversation_of(2, NONE, 77);
		CHECK(ai_conversation_line_begin(0), "the line is not begun");
		CHECK(conversation.line_unit_index == NONE && conversation.line_unspatialized,
			"one of two participants speaks it (%lx)", conversation.line_unit_index);
		return 0;
	}
	CASE("no dialogue")
	{
		conversation_of(1, NONE, NONE);
		CHECK(!ai_conversation_line_begin(0), "a line with no dialogue is begun");
		return 0;
	}
	CASE("named line")
	{
		conversation_of(1, 0, 77);
		conversation.participant_bitmask = 1;
		conversation.actor_indices[0] = ACTOR;
		conversation.dialogue_indices[0] = 0;
		CHECK(ai_conversation_line_begin(0), "the line is not begun");
		CHECK(conversation.line_unit_index == OTHER_UNIT && conversation.line_actor_index == ACTOR &&
			conversation.line_sound_index == 77 && conversation.line_participant_index == 0,
			"not begun as the participant's line");
		return 0;
	}
	CASE("participant not found")
	{
		conversation_of(1, 0, 77);
		CHECK(!ai_conversation_line_begin(0), "the line of a participant not found is begun");
		return 0;
	}
	return 0;
}
