"""The real ai_conversation_line_begin (ai_communication.c): a line of an AI conversation that names no participant
(a50's prison_kickin_ass, the corporal's line as the cell's Covenant fall, which was skipped) is spoken by the
conversation's one participant's named unit while it lives, else unspatialized; one with no dialogue is skipped; a
participant's line is begun as before, and one of a participant not found is skipped as before."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run, structure

CASES = ['unnamed line', 'dead participant', 'two participants', 'no dialogue', 'named line',
         'participant not found']
CONTROLS = [
    # (the line skipped, as before)
    ('else if (participant_index == NONE)', 'else if (FALSE)', 'unnamed line'),
    # (spoken by a dead unit)
    ('if (unit && !TEST_FLAG(unit->object.damage_flags, _object_dead_bit))', 'if (unit)', 'dead participant'),
    # (spoken by the first of several participants)
    ('if (ai_conversation_participant_count(definition) == 1)', 'if (TRUE)', 'two participants'),
    # (begun with no dialogue to speak)
    ('if (variant_index < MAXIMUM_DIALOGUE_VARIANTS_PER_CONVERSATION_PARTICIPANT)\n\t\t{\n\t\t\tconversation->line_participant_index = NONE;',
     'if (TRUE)\n\t\t{\n\t\t\tconversation->line_participant_index = NONE;', 'no dialogue'),
]
COMMUNICATION = 'source/ai/ai_communication.c'


def generated(fault=None):
    communication = read(COMMUNICATION)
    # (the counts, and the address and selection kinds the line reads, as the file's enums declare them)
    enums = []
    for member in ['MAXIMUM_DIALOGUE_VARIANTS_PER_CONVERSATION_PARTICIPANT', '_ai_conversation_address_player',
                   '_ai_conversation_selection_radio_sargeant']:
        enum = enum_with(communication, member)
        if enum not in enums:
            enums.append(enum)
    config = '\n'.join(
        ['typedef unsigned short word;',
         'struct tag_reference { long group_tag; long name; long name_length; long index; };',
         'struct tag_block { long count; void *address; long definition; };',
         enum_with(read('source/objects/objects.h'), '_object_dead_bit')] + enums +
        [structure(read('source/ai/ai_scenario_definitions.h'), 'ai_conversation'),
         structure(read('source/ai/ai_communication.h'), 'conversation_datum'),
         structure(communication, 'scenario_conversation_participant_view'),
         structure(communication, 'scenario_conversation_line_view')]) + '\n'
    code = '\n'.join(function(communication, name) for name in
                     ['ai_conversation_participant_count', 'ai_conversation_line_begin'])
    if fault:
        code = mutated(code, *fault)
    return (('config.inc', config), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('conversation_unnamed_line', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('before,after,case', CONTROLS)
def test_negative_control(before, after, case):
    status, output = run(build('conversation_unnamed_line', generated((before, after))), case)
    assert status == CHECK_FAILED, output
