from pathlib import Path
import collections
from typing import List
import typing


def count_tokens_in_file(path: Path, tokens: List[str]) -> typing.Counter[str]:
    """Count strings in a textfile

    Args:
        path:       Path to file
        tokens:     List of strings to look for

    Returns:
        The number of occurrences per string

    """
    result: typing.Counter[str] = collections.Counter()

    file_contents = path.read_text()
    for token in tokens:
        hits = file_contents.count(token)
        result.update({token: hits})

    return result


##### Public API #####


def count_tokens_in_directory(
    directorypath: Path, filepatterns: List[str], tokens: List[str]
) -> typing.Counter[str]:
    """Count strings in textfiles in a directory

    Args:
        path:           Path to directory
        filepatterns:   Glob pattern to select files in the directory
        tokens:         List of strings to look for

    Returns:
        The number of occurrences per string

    """
    result: typing.Counter[str] = collections.Counter()

    directory = Path(directorypath).absolute().resolve()

    paths: List[Path] = []
    for pattern in filepatterns:
        paths.extend(list(directory.glob(pattern)))

    for path in paths:
        result += count_tokens_in_file(path, tokens)

    return result
