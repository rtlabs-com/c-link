import argparse
import pathlib

import librequirements

# For type checking, use:
#   mypy --strict parse_requirements.py librequirements.py

DEFAULT_REQUIREMENTS_FILE = "implementation_requirements.csv"
DEFAULT_LOCREPORT_FILE = "req_testlocation_report.rst"
DEFAULT_REQREPORT_FILE = "requirement_list_report.rst"
DEFAULT_TESTCASEREPORT_FILE = "testcase_report.rst"
DEFAULT_SPECREPORT_FILE = "specification_report.rst"
DEFAULT_HOVER_DEFINITION_FILE = "vscode_hover.txt"
DEFAULT_OUT_SUBDIR = "_generated"

parser = argparse.ArgumentParser("Generate requirements report for CC-Link")
parser.add_argument(
    "xml",
    help="Path to directory with Doxygen XML files.",
)
parser.add_argument(
    "--req",
    help="Path to requirements CSV file. Defaults to "
    f"{DEFAULT_REQUIREMENTS_FILE} in the same directory as this script",
)
parser.add_argument(
    "--lrep",
    help="Path to resulting location report file. Defaults to "
    f"{DEFAULT_LOCREPORT_FILE} in the subdir {DEFAULT_OUT_SUBDIR} "
    "of the directory containing this script",
)
parser.add_argument(
    "--rrep",
    help="Path to resulting requirement list report file. Defaults to "
    f"{DEFAULT_REQREPORT_FILE} in the subdir {DEFAULT_OUT_SUBDIR} "
    "of the directory containing this script",
)
parser.add_argument(
    "--trep",
    help="Path to resulting testcase list report file. Defaults to "
    f"{DEFAULT_TESTCASEREPORT_FILE} in the subdir {DEFAULT_OUT_SUBDIR} "
    "of the directory containing this script",
)
parser.add_argument(
    "--srep",
    help="Path to resulting specification report file. Defaults to "
    f"{DEFAULT_SPECREPORT_FILE} in the subdir {DEFAULT_OUT_SUBDIR} "
    "of the directory containing this script",
)
parser.add_argument(
    "--hover",
    help="Path to resulting hover definition file. Defaults to "
    f"{DEFAULT_HOVER_DEFINITION_FILE} in the subdir {DEFAULT_OUT_SUBDIR} "
    "of the directory containing this script",
)
args = parser.parse_args()

script_directory = pathlib.Path(__file__).parent.resolve()
if args.req is None:
    args.req = script_directory.joinpath(DEFAULT_REQUIREMENTS_FILE)
if args.rrep is None:
    args.rrep = script_directory.joinpath(DEFAULT_OUT_SUBDIR, DEFAULT_REQREPORT_FILE)
if args.trep is None:
    args.trep = script_directory.joinpath(
        DEFAULT_OUT_SUBDIR, DEFAULT_TESTCASEREPORT_FILE
    )
if args.lrep is None:
    args.lrep = script_directory.joinpath(DEFAULT_OUT_SUBDIR, DEFAULT_LOCREPORT_FILE)
if args.srep is None:
    args.srep = script_directory.joinpath(DEFAULT_OUT_SUBDIR, DEFAULT_SPECREPORT_FILE)
if args.hover is None:
    args.hover = script_directory.joinpath(
        DEFAULT_OUT_SUBDIR, DEFAULT_HOVER_DEFINITION_FILE
    )

print("Requirements file: ", args.req)
print("XML directory: ", args.xml)
print("Requirement list report file: ", args.rrep)
print("Test case list report file: ", args.trep)
print("Requirement location report file: ", args.lrep)
print("Specification report file: ", args.srep)
print("Hover definition file: ", args.hover)

librequirements.analyze_requirements(
    args.req,
    args.xml,
    ["cl_*c.xml", "clm_*c.xml", "cls_*c.xml"],
    ["test_*.xml"],
    args.rrep,
    args.trep,
    args.lrep,
    args.srep,
    args.hover,
)
