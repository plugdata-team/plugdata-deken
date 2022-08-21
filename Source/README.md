# PlugDataDekenServer
Server that hosts all available packages for Pd's deken server.

This repo automatically creates an XML representation for the available packages in the pd-deken repository. Since requesting all information about objects from deken takes a long time, I decided it would be better to pre-parse it to a compact format. To further reduce download time, they are separated by OS and architecture.

This repo generates both an XML and binary representation, one to be humanly readable and one that is more compressed.

Check the "updater" branch to view the source code of the script!
