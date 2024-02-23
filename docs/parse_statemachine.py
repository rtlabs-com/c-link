import sys

# Verify Python version
assert sys.version_info >= (3, 6)

import pathlib
import re
from typing import Dict, List, Tuple

import graphviz


def get_table(fulltext: str, tablename: str, number_of_columns: int) -> List[List[str]]:
    """Parse a C definition table

    The table should have this format:

       whatever tablename[] = {
          {abc, def,ghi, jkl},
          /* My C-style comment */
          {mno,pqr,   stuv, xyz1},
       };

    Spaces and newlines does not matter.
    All entries should have the same number of sub-entries ("columns").

    Return a list of rows, where each row is a list of strings.
    """
    cleaned_text = fulltext.replace("\n", "").replace(" ", "")

    # Find table contents
    tableposition = cleaned_text.find(tablename + "[]")
    if tableposition == -1:
        raise ValueError(f'Could not find table "{tablename}" in the source code')
    first_curly_pos = cleaned_text.find("{", tableposition + 1)
    endposition = cleaned_text.find(";", tableposition)
    table = cleaned_text[first_curly_pos:endposition]

    # Remove C-style comments /* My comment */
    table = re.sub(re.compile("/\*.*?\*/", re.DOTALL), "", table)

    # Remove leading and trailing brackets
    table = table.lstrip("{")
    table = table.rstrip("}")
    table = table.rstrip(",")
    table = table.rstrip("}")

    # Read table cells
    result = []
    rows = table.split("},{")
    for row in rows:
        split_row = row.split(",")
        len_split_row = len(split_row)
        if len_split_row != number_of_columns:
            raise ValueError(
                f'Wrong number of columns in row for table "{tablename}". There are {len_split_row}, but should be {number_of_columns}: {split_row}'
            )
        result.append(split_row)

    return result


def remove_prefix(prefix: str, inputstring: str) -> str:
    if inputstring.startswith(prefix):
        return inputstring[len(prefix) :]
    return inputstring


def filter_function_name(functionname: str) -> str:
    return functionname if functionname != "NULL" else ""


def add_parenthesis_to_functionname(functionname: str) -> str:
    if functionname == "":
        return ""
    return functionname + "()"


def generate_node_label(statename: str, on_entry: str, on_exit: str) -> str:
    return (
        f'<<B>{statename}</B><BR ALIGN="CENTER"/>'
        + f'On entry: {on_entry}<BR ALIGN="LEFT"/>'
        + f'On exit: {on_exit}<BR ALIGN="LEFT"/>>'
    )


def generate_edge_label(eventname: str, functionname: str) -> str:
    if functionname:
        return eventname + ":\n" + functionname
    return eventname


def calculate_node_color(eventname: str) -> str:
    return "red" if "CYCLIC" in eventname else "black"


class Transition:
    def __init__(
        self, start_state: str, end_state: str, event: str, functionname: str
    ) -> None:
        self.start_state = start_state
        self.end_state = end_state
        self.event = event
        self.functionname = functionname


class StateInfo:
    def __init__(self, state: str, on_entry: str = "", on_exit: str = "") -> None:
        self.state = state
        self.on_entry = on_entry
        self.on_exit = on_exit

    def __repr__(self) -> str:
        return f"State {self.state} On entry '{self.on_entry}' On exit '{self.on_exit}'"


def get_states_and_transitions(
    source_code: str,
    transistion_table_name: str,
    entry_exit_table_name: str,
) -> Tuple[Dict[str, StateInfo], List[Transition]]:

    transition_table = get_table(source_code, transistion_table_name, 4)
    transitions = []
    for row in transition_table:
        transitions.append(
            Transition(row[0], row[2], row[1], filter_function_name(row[3]))
        )

    state_names = set(
        [x.start_state for x in transitions] + [x.end_state for x in transitions]
    )
    state_infos = {x: StateInfo(x) for x in state_names}

    entry_exit_table = get_table(source_code, entry_exit_table_name, 3)
    for row in entry_exit_table:
        state_name = row[0]
        if state_name not in state_infos:
            state_infos[state_name] = StateInfo(state_name)

        state_infos[state_name].on_entry = filter_function_name(row[1])
        state_infos[state_name].on_exit = filter_function_name(row[2])

    return state_infos, transitions


def save_dot_file(
    state_infos: Dict[str, StateInfo],
    transitions: List[Transition],
    figure_size: str,
    state_prefix: str,
    event_prefix: str,
    rankdir: str,
    label: str,
    filename: str,
) -> None:
    graph = graphviz.Digraph(
        comment=label,
        graph_attr={
            "rankdir": rankdir,
            "size": figure_size,
            "label": label,
            "labelloc": "t",
            "fontsize": "40",
        },
        node_attr={"fillcolor": "lightcyan", "style": "filled"},
    )

    # Add nodes
    for state_name in state_infos:
        label = generate_node_label(
            remove_prefix(state_prefix, state_name),
            add_parenthesis_to_functionname(state_infos[state_name].on_entry),
            add_parenthesis_to_functionname(state_infos[state_name].on_exit),
        )
        graph.node(state_name, label)

    # Add edges
    for transition in transitions:
        label = generate_edge_label(
            remove_prefix(event_prefix, transition.event),
            add_parenthesis_to_functionname(transition.functionname),
        )
        graph.edge(
            transition.start_state,
            transition.end_state,
            label,
            fontcolor=calculate_node_color(transition.event),
            color=calculate_node_color(transition.event),
        )

    graph.save(filename)


def parse_state_statemachine(
    label: str,
    source_code_path: str,
    transistion_table_name: str,
    entry_exit_table_name: str,
    figure_size: str,
    rankdir: str,
    state_prefix: str,
    event_prefix: str,
    filename: str,
) -> None:
    source_code = pathlib.Path(source_code_path).read_text()
    state_infos, transitions = get_states_and_transitions(
        source_code, transistion_table_name, entry_exit_table_name
    )
    save_dot_file(
        state_infos,
        transitions,
        figure_size,
        state_prefix,
        event_prefix,
        rankdir,
        label,
        filename,
    )


# The rankdir can be "TB" or "LR". See graphviz documentation.
def main():
    script_directory = pathlib.Path(__file__).resolve().parent

    parse_state_statemachine(
        "Slave state transitions",
        script_directory.parent / "src/slave/cls_iefb.c",
        "transitions",
        "state_actions",
        "30,40",
        "TB",
        "CLS_SLAVE_STATE_",
        "CLS_SLAVE_EVENT_",
        script_directory / "_generated/slave_state_machine.dot",
    )

    parse_state_statemachine(
        "Device state transitions",
        script_directory.parent / "src/master/clm_iefb.c",
        "device_transitions",
        "device_state_actions",
        "30,40",
        "TB",
        "CLM_DEVICE_STATE_",
        "CLM_DEVICE_EVENT_",
        script_directory / "_generated/device_state_machine.dot",
    )

    parse_state_statemachine(
        "Group state transitions",
        script_directory.parent / "src/master/clm_iefb.c",
        "group_transitions",
        "group_state_actions",
        "30,40",
        "TB",
        "CLM_GROUP_STATE_",
        "CLM_GROUP_EVENT_",
        script_directory / "_generated/group_state_machine.dot",
    )


if __name__ == "__main__":
    main()

# To convert to PNG file:
#     dot -Tpng device_state_machine.dot -o device_state_machine.png
#     dot -Tpng slave_state_machine.dot -o slave_state_machine.png
#     dot -Tpng group_state_machine.dot -o group_state_machine.png
