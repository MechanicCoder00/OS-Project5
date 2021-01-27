Changelog

commit 37da27c99b148fdeb12b8d7920af7ade3193be63
Author: Scott Tabaka <tabaka@hoare7.cs.umsl.edu>
Date:   Sun Apr 12 17:29:54 2020 -0500

    Forgot to upload update Readme

 README | 19 ++++++++++++++++---
 oss.c  |  7 -------
 2 files changed, 16 insertions(+), 10 deletions(-)

commit c2d52b50d1e10fbe8e9bc9bb6635642aadbbe21b
Author: Scott Tabaka <tabaka@hoare7.cs.umsl.edu>
Date:   Sun Apr 12 17:26:33 2020 -0500

    Finalized readme,comments, added logfile limit

 oss.c     | 229 ++++++++++++++++++++++++++++++++++++++++++++--------------------------------------
 process.c | 136 ++++++++++++++++++++++++++++++++-----------------
 2 files changed, 211 insertions(+), 154 deletions(-)

commit b6b5b0341ab6307037b896a7cf779835a9e9a7dd
Author: Scott Tabaka <tabaka@hoare7.cs.umsl.edu>
Date:   Sun Apr 12 00:03:22 2020 -0500

    Cleared out most debugging comments

 oss.c     | 445 ++++++++++++++++++----------------------------------------------------------------
 process.c | 111 ++-------------------
 2 files changed, 107 insertions(+), 449 deletions(-)

commit 4857b51ea922f85262727f5650f2cc04f47f3f2d
Author: Scott Tabaka <tabaka@hoare7.cs.umsl.edu>
Date:   Sat Apr 11 22:46:29 2020 -0500

    Added statistics

 oss.c     | 285 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++---------------
 process.c |  15 +++--
 2 files changed, 243 insertions(+), 57 deletions(-)

commit f86cb92b7e8c960c02d46879cc8a7df67773dcdc
Author: Scott Tabaka <tabaka@hoare7.cs.umsl.edu>
Date:   Sat Apr 11 19:55:27 2020 -0500

    Added output to log file

 oss.c     | 489 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++------------------
 process.c | 239 ++++++++++++++++++++++++++++++++++++----
 2 files changed, 602 insertions(+), 126 deletions(-)

commit 7066f0804b07fcbd1e76249cabe542b8e883d276
Author: Scott Tabaka <tabaka@hoare7.cs.umsl.edu>
Date:   Wed Apr 8 22:44:01 2020 -0500

    Added deadlock resolution

 oss.c     | 480 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++-----------------------
 process.c | 177 ++++++++++++++++++++----------
 2 files changed, 467 insertions(+), 190 deletions(-)

commit 888214c6a53841a8041880e9e828689baef1648c
Author: Scott Tabaka <tabaka@hoare7.cs.umsl.edu>
Date:   Tue Apr 7 10:38:43 2020 -0500

    Added deadlock detection

 oss.c     | 346 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++--------------------------
 process.c |  58 ++++++++------
 2 files changed, 273 insertions(+), 131 deletions(-)

commit 3d27d6c25bbed4fa571069326ee0f4972194c248
Author: Scott Tabaka <tabaka@hoare7.cs.umsl.edu>
Date:   Sun Apr 5 17:07:42 2020 -0500

    Requesting,allocating,and deallocating working

 oss.c     | 472 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++------------------
 process.c | 152 +++++++++++++++++++++------
 2 files changed, 486 insertions(+), 138 deletions(-)

commit 0db1f7d16fb896d4f9983c74748b30b0a42ec3f2
Author: Scott Tabaka <tabaka@hoare7.cs.umsl.edu>
Date:   Sat Apr 4 17:25:31 2020 -0500

    Added resource descriptors

 oss.c     | 247 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++----------
 process.c |  44 +++++++++++----
 2 files changed, 251 insertions(+), 40 deletions(-)

commit 23809de828fc78f76dbe962f261137aa5ec92803
Author: Scott Tabaka <tabaka@hoare7.cs.umsl.edu>
Date:   Fri Apr 3 13:50:36 2020 -0500

    Initial commit

 Makefile     |  24 +++++++++
 README       |  14 +++++
 changelog.md |   2 +
 oss.c        | 224 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 process.c    | 114 ++++++++++++++++++++++++++++++++++++++++
 5 files changed, 378 insertions(+)
