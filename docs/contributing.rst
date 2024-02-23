Contribution guide
==================

Contributions are welcome. If you want to contribute you will need to
sign a Contributor License Agreement and send it to us either by
e-mail or by physical mail. More information is available `here`_.

.. _here: https://rt-labs.com/contribution

Pull requests
-------------

Pull requests are reviewed before merging. Reviewing pull requests can
take a lot of time so to simplify the work of the reviewer we ask that
the following guideline is followed.

A PR should implement a feature or a bugfix and should be complete,
that is, no further work on the feature or bugfix is expected. Mark
the PR as a work-in-progress (draft) if you wish to get early feedback
but have not yet finished the PR.

A PR should include a reference to the issue tracking the feature or
bugfix. Create an issue if one does not exist already.

A PR should consist of a series of logical commits. You are expected
to squash your commits so that they follow this rule. For instance, if
after a review you are asked to make changes then these changes should
be squashed into the corresponding logical commit.

All commits in the PR should have commit messages that explain what
was changed and why it was changed, including references to relevant
issues. There are many online guides to writing good commit messages
[1]_ [2]_.

A PR should include tests. If you are fixing a bug then add a
test-case that triggers the bug and will pass once the bug is
fixed. If you are adding a new feature then the PR should contain
tests for the feature. The PR should not lower the code coverage level
of the test suites.

A PR should pass all prior tests. Note that this is checked when
submitting although it is useful to run the tests locally first.

Coding style
------------

Familiarize yourself with the code base and follow the same patterns.

Run clang-format on staged files before committing::

    git add .
    git clang-format

This will format the commit using clang-format. Examine and stage
modified files before finalizing the commit.

Public functions in a module should use a common prefix, typically
derived from the name of the module. Private functions can use the
same prefix too.

Use reasonably short yet descriptive [3]_ names for functions and
variables. Separate words using underscore (also known as "snake-case"
[4]_).

Do not use hungarian notation [5]_.

Function names should follow the pattern `prefix_verb` or
`prefix_object_verb` if possible.

Names of boolean variables should follow the pattern `is_condition` or
`has_attribute`.

A function should in most cases not have more than three levels of
indentation. Refactor the code to avoid this.

.. highlight:: C

Functions should follow the return early pattern [6]_. This makes the
happy path easier to follow and keeps refactoring diffs to a minimum::

  int mdl_do_work (mdl_t * mdl)
  {
     if (mdl == NULL)
     {
        return -1;
     }

     /* Do the work */

     return 0;
  }

Do not use "Yoda conditions" [7]_::

    if (3 == a) { /* ... */ }

Use C-style comments in C files. Use C or C++ comments in C++ files.

Goto should not be used, except for in structured error handling code
[8]_. For this particular scenario only, goto is considered useful::

  mdl_t * mdl_create (void)
  {
     mdl_t * mdl;

     mdl = malloc (sizeof (*mdl));
     if (mdl == NULL)
     {
        return -1;
     }

     mdl->resource1 = mdl_resource_create();
     if (mdl->resource1 == NULL)
     {
        goto exit1;
     }

     mdl->resource2 = mdl_resource_create();
     if (mdl->resource2 == NULL)
     {
        goto exit2;
     }

     /* ... */

     return 0;

  exit2:
     mdl_resource_free (mdl->resource1);
  exit1:
     free (mdl);
     return -1;
  }

Use the ``CC_ASSERT`` port-specific macro for assertions. The
``assert()`` function is not always available on embedded platforms,
or it might be preferable to use some other mechanism.

.. [1] https://cbea.ms/git-commit/
.. [2] https://joshuatauberer.medium.com/write-joyous-git-commit-messages-2f98891114c4
.. [3] These may be conflicting requirements. Naming things is
       difficult.
.. [5] https://en.wikipedia.org/wiki/Snake_case
.. [4] https://en.wikipedia.org/wiki/Hungarian_notation
.. [6] https://medium.com/swlh/return-early-pattern-3d18a41bba8
.. [7] https://en.wikipedia.org/wiki/Yoda_conditions
.. [8] https://eli.thegreenplace.net/2009/04/27/using-goto-for-error-handling-in-c
