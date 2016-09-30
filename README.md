SourceSync
==========

SourceSync is a standalone binary that is aimed at synchronizing
source code to a remote
system.

The specific use case it was designed for was to edit code on MacOS,
but compile it in a docker container. Since the docker host typically
does not run as a file server (CIFS) my normal workflow (source on
comile host, edit over CIFS) wasn't available.

After an initial synchronization, editing a single file is updated
on the remote host in about 1second - quicker than I can change
windows and type make.

The sychronization requires the rsync binary on the remote end and
a SSH connection to the remote host


Running
=======

Testing
-------

```
[ morgaine ] ./test-srcsync -v10 --auth=richard
Initializing subsystem: Logging Subsystem
./srcsync --from=TEST-SRC-DIR/ --to=ssh://richard@localhost/Users/richard/SRC/WhiteQueen/srcsync/BUILD/TEST-DST-DIR/ --ignore=*.IGNORE
19:34:43 [N] Syncing from TEST-SRC-DIR/ to ssh://richard@localhost/Users/richard/SRC/WhiteQueen/srcsync/BUILD/TEST-DST-DIR/
19:34:43 [N] Processing initial synchronization
19:34:43 [N] worker-1: Updated /Users/richard/SRC/WhiteQueen/srcsync/BUILD/TEST-SRC-DIR/.
19:34:48 [N] Initial synchronization complete
[==========] Running 3 tests from 1 test case.
[----------] Global test environment set-up.
[----------] 3 tests from NewFile
[ RUN      ] NewFile.newZeroLengthFile
19:34:53 [N] worker-0: Updated /Users/richard/SRC/WhiteQueen/srcsync/BUILD/TEST-SRC-DIR/1475202893640615
[       OK ] NewFile.newZeroLengthFile (5004 ms)
[ RUN      ] NewFile.newNonZeroLengthFile
19:34:58 [N] worker-2: Updated /Users/richard/SRC/WhiteQueen/srcsync/BUILD/TEST-SRC-DIR/1475202898644370
[       OK ] NewFile.newNonZeroLengthFile (5002 ms)
[ RUN      ] NewFile.newIgnoredZeroLengthFile
19:35:04 [N] worker-3: Ignoring 1475202903646491.IGNORE
[       OK ] NewFile.newIgnoredZeroLengthFile (5001 ms)
[----------] 3 tests from NewFile (15007 ms total)

[----------] Global test environment tear-down
[==========] 3 tests from 1 test case ran. (15007 ms total)
[  PASSED  ] 3 tests.
Uninitializing subsystem: Logging Subsystem
cleanup
```

License
=======

Not good, mixed in reciprocal license (bad) and GPL... needs to be resolved.

-- 
Richard Offer <richard@whitequeen.com>
