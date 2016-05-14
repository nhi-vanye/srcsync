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


License
=======

Not good, mixed in reciprocal license (bad) and GPL... needs to be resolved.

-- 
Richard Offer <richard@whitequeen.com>
