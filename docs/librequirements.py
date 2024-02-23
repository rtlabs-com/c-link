import collections
import csv
import dataclasses
import pathlib
from typing import DefaultDict, List, Set, Tuple
import xml.etree.ElementTree as ET

from rstcloth import RstCloth  # type: ignore


@dataclasses.dataclass(frozen=True)
class Requirement:
    id: str
    description: str
    specifications: List[str]

    def _get_id_parts(self) -> Tuple[str, int]:
        id_parts = self.id.rsplit("_", maxsplit=1)
        if len(id_parts) != 2:
            raise ValueError(f"This requirement ID has wrong format: {self.id}")

        prefix = id_parts[0]
        try:
            number = int(id_parts[1])
        except:
            raise ValueError(f"This requirement ID has wrong number format: {self.id}")

        return prefix, number

    @property
    def id_prefix(self) -> str:
        """Get the prefix part of the requirement ID"""
        return self._get_id_parts()[0]

    @property
    def id_number(self) -> int:
        """Get the number (last) part of the requirement ID"""
        return self._get_id_parts()[1]


@dataclasses.dataclass(frozen=True)
class FunctionDescription:
    name: str
    description: str
    path: str
    startline: int
    fixturename: str
    req_ids: List[str]

    def __hash__(self) -> int:
        return hash(self.name)


ReqLocations = DefaultDict[str, List[FunctionDescription]]
"""Dictionary with a list of locations for each requirement."""


def parse_requirement_file(path: pathlib.Path) -> List[Requirement]:
    """Parse a requirements file in CSV format.

    Separator is comma
    Strings should be quoted if they contain comma.
    The columns should be: ID, Description, Specifications
    The specifications column is a comma separated string of specification locations.

    Each ID should be of the form IDPREFIX_IDNUMBER, for example REQ_CLS_STATUSBIT_03
    where IDPREFIX is REQ_CLS_STATUSBIT and IDNUMBER is 03

    First row is ignored.

    Args:
        path:   Path to file to be parsed

    Returns:
        List of requirements, sorted by requirement ID

    """
    result = []

    with open(path, newline="") as csvfile:
        reader_for_csv = csv.reader(csvfile, delimiter=",")
        for line_no, line in enumerate(reader_for_csv):
            if line_no == 0:
                continue
            if len(line) != 3:
                raise ValueError(
                    f"All lines in the requirements CSV file must have three columns! Line: {line}"
                )
            specifications = [x.strip() for x in line[2].split(",") if x.strip()]
            id = line[0].strip().strip("?_")
            if id:
                result.append(Requirement(id, line[1].strip(), specifications))

    return sorted(result, key=lambda x: x.id)


def detect_requirement_duplicates(requirements: List[Requirement]) -> None:
    """Verify that there are no duplicates among the requirements

    Uses the requirement ID only.

    Args:
        requirements: Requirements to be verified

    Raises:
        ValueError if duplicates are found.

    """
    if not requirements:
        return

    id_counts = collections.Counter([x.id for x in requirements])
    most_common_id, num_occurances = id_counts.most_common(1)[0]
    if num_occurances > 1:
        raise ValueError(
            "There are duplicates in the requirement IDs, for example "
            f"'{most_common_id}' is found {num_occurances} times."
        )


def describe_missing_requirements(requirements: List[Requirement]) -> None:
    """Prints out a warning for requirements that have missing values

    Args:
        requirements: Requirements to be verified

    """
    # Extract the numbers from the requirements
    req_numbers: DefaultDict[str, List[int]] = collections.defaultdict(list)
    for req in requirements:
        req_numbers[req.id_prefix].append(req.id_number)

    # Verify that there are no missing numbers in the list of requirements
    for prefix in req_numbers:
        found_numbers = sorted(req_numbers[prefix])
        expected_numbers = list(range(1, len(found_numbers) + 1))
        if found_numbers != expected_numbers:
            print(
                f"  Unusual sequence numbers for '{prefix}' requirements: {found_numbers}"
            )


def find_specification_ids(requirements: List[Requirement]) -> List[str]:
    """Find all specification IDs used in the requirements

    Return:
        A sorted list of specification IDs.

    """
    spec_ids = set()

    for req in requirements:
        spec_ids.update(req.specifications)

    return sorted(spec_ids)


def parse_doxygen_xmlfile(
    path: pathlib.Path, is_testcase_file: bool
) -> List[FunctionDescription]:
    """Parse a doxygen XML file to find requirements in docstrings.

    Args:
        path: Path to the XML file to be parsed
        is_testcase_file: True if a testcase file (Will then read testfixture name).

    Return:
        Found function descriptions

    """
    outputresult = []

    tree = ET.parse(str(path))
    root = tree.getroot()

    for function_element in root.findall(".//memberdef[@kind='function']"):
        testfixture_name = ""
        testcase_name = ""
        function_name = ""
        description = ""
        req_ids = []

        function_name_element = function_element.find("./name")
        if function_name_element is not None and function_name_element.text:
            function_name = function_name_element.text
        function_location_element = function_element.find("./location")
        if function_location_element is not None:
            filepath = function_location_element.attrib["file"]
            try:
                lineno = int(function_location_element.attrib["line"])
            except:
                lineno = 0
        first_line_element = function_element.find("./detaileddescription/para[1]")
        if first_line_element is not None and first_line_element.text:
            description = first_line_element.text

        xref_elememts = function_element.findall("./detaileddescription/para/xrefsect")
        for xref_elem in xref_elememts:
            xref_title_elem = xref_elem.find(".xreftitle")
            if xref_title_elem is not None and xref_title_elem.text == "Requirement":
                req_ids = [
                    a.text.strip()
                    for a in xref_elem.findall(".xrefdescription/para")
                    if a is not None and a.text
                ]

        if is_testcase_file:
            testfixture_elem = function_element.find("./param[1]/type/ref")
            if testfixture_elem is None:
                testfixture_elem = function_element.find("./param[1]/type")
            if testfixture_elem is not None and testfixture_elem.text:
                testfixture_name = testfixture_elem.text
            testcase_element = function_element.find("./param[2]/type")
            if testcase_element is not None and testcase_element.text:
                testcase_name = testcase_element.text

        if is_testcase_file:
            new_item = FunctionDescription(
                testcase_name, description, filepath, lineno, testfixture_name, req_ids
            )
        else:
            new_item = FunctionDescription(
                function_name, description, filepath, lineno, "", req_ids
            )
        outputresult.append(new_item)

    return outputresult


def parse_doxygen_files(
    directorypath: str, patterns: List[str], is_testcase_file: bool
) -> List[FunctionDescription]:
    """Parse all Doxygen XML files in a directory.

    Args:
        directorypath: Path to directory containing XML files
        patterns: Glob patterns to find relevant XML files
        is_testcase_file: True if the found files are expected to be testcase files
                          (will then parse the fixturename)

    Returns:
        Found function descriptions
    """
    result = []

    directory = pathlib.Path(directorypath)

    paths: List[pathlib.Path] = []
    for pattern in patterns:
        paths.extend(list(directory.glob(pattern)))

    for path in paths:
        result.extend(parse_doxygen_xmlfile(path, is_testcase_file))

    return result


def find_locations_per_requirement(
    descriptions: List[FunctionDescription],
) -> ReqLocations:
    """Find all locations per requirement

    Args:
        descriptions: A list of location descriptions

    Returns:
        Locations for each requirement IDs

    """
    result: ReqLocations = collections.defaultdict(list)

    for funcdescr in descriptions:
        for req_id in funcdescr.req_ids:
            result[req_id].append(funcdescr)

    return result


def find_unique_locations(input: ReqLocations) -> Set[FunctionDescription]:
    """Find unique locations

    Args:
        input: Locations for each requirement

    Returns:
        Unique locations

    """
    output = set()
    for req_id in input:
        output.update(input[req_id])
    return output


def find_invalid_tag_locations(
    requirements: List[Requirement],
    impl_locations: ReqLocations,
    testcases: ReqLocations,
) -> ReqLocations:
    """Find locations where invalid requirement IDs are used in the code

    Args:
        requirements:   Valid requirements
        impl_locations: Implementation locations with requirement tags
        testcases:      Test case locations with requirement tags

    Returns:
        Locations where invalid requirement IDs are used

    """
    outputresult: ReqLocations = collections.defaultdict(list)

    requirement_ids: Set[str] = set([x.id for x in requirements])
    impl_req_ids: Set[str] = set(impl_locations.keys())
    test_req_ids: Set[str] = set(testcases.keys())

    for id in sorted(impl_req_ids - requirement_ids):
        outputresult[id].extend(impl_locations[id])
    for id in sorted(test_req_ids - requirement_ids):
        outputresult[id].extend(testcases[id])

    return outputresult


def find_requirements_with_test_cases(
    requirements: List[Requirement],
    testcases: ReqLocations,
) -> Tuple[List[Requirement], List[Requirement]]:
    """Sort the requirements into a list of those with tests and a list for those without

    Args:
        requirements: Requirements.
        testcases: Test case locations for each requirement

    Returns two lists of requirements, one containing those that are covered
    by test cases and one containing those that are not.

    """
    requirements_with_testcases = []
    requirements_without_testcases = []
    for req in sorted(requirements, key=lambda x: x.id):
        if req.id in testcases:
            requirements_with_testcases.append(req)
        else:
            requirements_without_testcases.append(req)

    return requirements_with_testcases, requirements_without_testcases


class ReportWriter:
    def __init__(self, output_filepath: str, title: str) -> None:
        """Create a report in RST format.

        Args:
            output_filepath:    Path to resulting report
            title:              Report title

        """
        self.file = open(output_filepath, "w")
        self.report = RstCloth(self.file, line_width=120)
        self.report.title(title)

    def add_statistics(
        self,
        num_req_with_tests: int,
        num_req_without_tests: int,
        num_locations: int,
        num_tests: int,
    ) -> None:
        """Add statistics text to report

        Args:
            num_req_with_tests:     Number of requirements with test cases
            num_req_without_tests:  Number of requirements without test cases
            num_locations:          Total number of implementation locations
            num_tests:              Total number of test case locations

        """
        self.report.content(
            f"{num_req_with_tests+ num_req_without_tests} requirements in total, "
            f"out of which {num_req_without_tests} are not yet mapped to any test case."
        )
        self.report.content(
            f"Implemented in {num_locations} functions, and {num_tests} of the test cases have requirement tags."
        )

    def add_header(self, header: str) -> None:
        """Add a section header

        Args:
            header:     Header text

        """
        self.report.newline()
        self.report.h2(header)
        self.report.newline()

    def add_subheader(self, header: str) -> None:
        """Add a subsection header

        Args:
            header:     Header text

        """
        self.report.newline()
        self.report.h3(header)

    def add_text(self, text: str) -> None:
        """Add text to the report

        Args:
            text:     Text

        """
        self.report.content(text)
        self.report.newline()

    def add_requirement(self, req: Requirement) -> None:
        """Add text on one requirement

        Args:
            req: requirement

        """
        self.add_subheader(req.id)
        self.add_text(req.description)

        for spec in req.specifications:
            self.report.li(spec)

    def add_requirement_summary(self, req: Requirement) -> None:
        """Add summary on one requirement

        Args:
            req: requirement

        """
        self.report.li(f"**{req.id}** {req.description}")

    def add_requirement_summary_numberonly(self, req: Requirement) -> None:
        """Add summary on one requirement. Use the number part of the ID.

        Args:
            req: requirement

        """
        self.report.li(f"**{req.id_number:02}** {req.description}")

    def add_see_also_other_spec(self, other_spec: List[str]) -> None:
        """Add reference to other specification IDs

        Args:
            other_spec: List of other specification IDs
        """
        if other_spec:
            self.report.newline()
            self.report.li("See also: " + ", ".join(other_spec), indent=3)
            self.report.newline()

    def add_short_location(self, location: FunctionDescription) -> None:
        """Add a short location text.

        Args:
            location:   Implementation description

        """
        if location.fixturename:
            self.report.li(f"{location.path} {location.startline}")
        else:
            self.report.li(
                f"**{location.name}()** {location.path} {location.startline}"
            )

    def add_implementation_location(self, impl: FunctionDescription) -> None:
        """Add an implementation location to a requirement text.

        Args:
            impl:   Implementation description

        """
        self.report.li(f"**{impl.name}()** {impl.path} {impl.startline}")
        if impl.description:
            self.report.newline()
            self.report.li(impl.description, indent=3)
            self.report.newline()

    def add_testcase_location(
        self,
        test_case: FunctionDescription,
    ) -> None:
        """Add a testcase location to a requirement text.

        Args:
            test_case: Test case description

        """
        self.report.li(
            f"Test case **{test_case.name}** {test_case.fixturename} {test_case.path} {test_case.startline}"
        )
        if test_case.description:
            self.report.newline()
            self.report.li(test_case.description, indent=3)
            self.report.newline()

    def add_testcase_summary(
        self,
        test_case: FunctionDescription,
    ) -> None:
        """Add a testcase summary to a testcase list.

        Args:
            test_case: Test case description

        """
        if len(test_case.req_ids):
            self.report.li(f"{test_case.name} **{len(test_case.req_ids)}**")
        else:
            self.report.li(f"{test_case.name}")

    def finalize(self) -> None:
        """Finalize the report"""
        self.file.close()


def build_location_report(
    output_path: str,
    requirements: List[Requirement],
    implementation_descriptions: List[FunctionDescription],
    testcase_descriptions: List[FunctionDescription],
) -> None:
    """Build a requirement report

    Use a section for requirements covered by test cases, and another
    section for those requirements that not are covered by test cases,

    Inside each section the requirement IDs are used as headings, and
    a short requirement description is given. References to specifications,
    implementation positions and test cases are given.

    Args:
        output_path: Path to the resulting text file
        requirements: Requirements.
        implementation_descriptions: Function descriptions for implementation
        testcase_descriptions: Function descriptions for test cases

    """
    impl_locations = find_locations_per_requirement(implementation_descriptions)
    test_locations = find_locations_per_requirement(testcase_descriptions)

    req_with_tests, req_without_tests = find_requirements_with_test_cases(
        requirements, test_locations
    )
    invalid_tag_locations = find_invalid_tag_locations(
        requirements, impl_locations, test_locations
    )

    report = ReportWriter(output_path, "Requirement details")
    report.add_statistics(
        len(req_with_tests),
        len(req_without_tests),
        len(find_unique_locations(impl_locations)),
        len(find_unique_locations(test_locations)),
    )
    report.add_header("Requirements with test cases")
    for req in req_with_tests:
        report.add_requirement(req)
        for impl in impl_locations[req.id]:
            report.add_implementation_location(impl)
        for tc in test_locations[req.id]:
            report.add_testcase_location(tc)

    report.add_header("Requirements not yet mapped to automated tests")
    for req in req_without_tests:
        report.add_requirement(req)
        for impl in impl_locations[req.id]:
            report.add_implementation_location(impl)

    report.add_header("Invalid requirement tags")
    if invalid_tag_locations:
        for invalid_id in sorted(invalid_tag_locations.keys()):
            report.add_subheader(invalid_id)
            for location in invalid_tag_locations[invalid_id]:
                report.add_short_location(location)
    else:
        report.add_text("None found.")

    report.finalize()


def build_specification_report(
    output_path: str,
    requirements: List[Requirement],
) -> None:
    """Build a specification report

    Use the specification as heading, and put the
    corresponding requirements in a bullet list. Add
    references to other specifications, if any.

    Args:
        output_path: Path to the resulting text file
        requirements: Requirements.

    """

    report = ReportWriter(output_path, "Mapping from specification to requirements")

    for spec_id in find_specification_ids(requirements):
        report.add_subheader(spec_id)
        for req in requirements:
            if spec_id in req.specifications:
                report.add_requirement_summary(req)
                other_spec = set(req.specifications)
                other_spec.remove(spec_id)
                report.add_see_also_other_spec(sorted(other_spec))

    report.finalize()


def build_requirement_list_report(
    output_path: str,
    requirements: List[Requirement],
) -> None:
    """Build a report file listing the requirements

    Use the requirement prefix as heading, and put the
    corresponding requirements in a bullet list.

    Args:
        output_path: Path to the resulting text file
        requirements: Requirements.

    """

    report = ReportWriter(output_path, "List of requirements")

    req_per_prefix: DefaultDict[str, List[Requirement]] = collections.defaultdict(list)
    for req in requirements:
        req_per_prefix[req.id_prefix].append(req)

    for prefix in sorted(req_per_prefix.keys()):
        report.add_subheader(prefix)
        found_requirements = sorted(req_per_prefix[prefix], key=lambda x: x.id)
        for req in found_requirements:
            report.add_requirement_summary_numberonly(req)

    report.finalize()


def build_testcase_list_report(
    output_path: str,
    testcase_descriptions: List[FunctionDescription],
) -> None:
    """Build a report file listing the requirements

    Use the requirement prefix as heading, and put the
    corresponding requirements in a bullet list.

    Args:
        output_path: Path to the resulting text file
        requirements: Requirements.

    """

    report = ReportWriter(output_path, "List of test cases")
    report.add_text(
        "The headers are the fixture names. The number of requirements "
        "verified by the test case (if any) are given on the same "
        "line as the test case name."
    )

    descriptions_per_fixture: DefaultDict[
        str, List[FunctionDescription]
    ] = collections.defaultdict(list)

    for testcase in testcase_descriptions:
        descriptions_per_fixture[testcase.fixturename].append(testcase)

    for fixturename in sorted(descriptions_per_fixture.keys()):
        report.add_subheader(fixturename)
        for testcase in sorted(
            descriptions_per_fixture[fixturename], key=lambda x: x.name
        ):
            report.add_testcase_summary(testcase)
    report.finalize()


def build_vscode_hover_definitions(
    output_path: str,
    requirements: List[Requirement],
) -> None:
    """Build a definition file for use with our hover extension in VS Code.

    The line format is::

        HOVERTAG,DESCRIPTION_IN_MARKDOWN

    Args:
        output_path: Path to the resulting text file
        requirements: Requirements.

    """
    with open(output_path, "w") as outputfile:
        for req in requirements:
            line: str = req.id + "," + req.description
            for spec in req.specifications:
                line += " <br> - **" + spec + "** "
            line += "\n"
            outputfile.write(line)


########### Public API #################


def analyze_requirements(
    requirements_path: str,
    xml_directory: str,
    implementation_file_patterns: List[str],
    test_file_patterns: List[str],
    requirementlist_report_path: str,
    testcase_report_path: str,
    location_report_path: str,
    specification_report_path: str,
    hover_definition_path: str,
) -> None:
    """Analyze requirements

    Args:
        requirements_path: Path to CSV file with requirements
        xml_directory: Path to directory containing XML files
        implementation_file_patterns: Glob patterns to find implementation XML files
        test_file_patterns: Glob patterns to find testcase XML files
        requirementlist_report_path: Path to resulting requirement list report
        location_report_path: Path to resulting location report
        specification_report_path: Path to resulting specification report
        hover_definition_path: Path to resulting definition file for our VSCode hover extension

    """
    requirements = parse_requirement_file(pathlib.Path(requirements_path))
    build_specification_report(specification_report_path, requirements)
    build_vscode_hover_definitions(hover_definition_path, requirements)
    build_requirement_list_report(requirementlist_report_path, requirements)

    detect_requirement_duplicates(requirements)
    describe_missing_requirements(requirements)

    implementations = parse_doxygen_files(
        xml_directory,
        implementation_file_patterns,
        False,
    )
    testcases = parse_doxygen_files(xml_directory, test_file_patterns, True)

    build_testcase_list_report(testcase_report_path, testcases)
    build_location_report(
        location_report_path, requirements, implementations, testcases
    )
