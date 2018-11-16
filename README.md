# pscom

0. help
	- [x] supported formats (pscom::sf)
	- [x] version of cli and lib (pscom::vi)
1. qDebug/qInfo/qWarning/qCritical/qFatal (VerbosityHandler)
	- [x] stderr, stdout
	- [x] "--verbosity"
2. progessbar
	- [x]
3. warn on lossy operations (replace, change, override, remove)
	- [ ] ask user for confirmation
	- [x] --dry-run mode showing consequences
	- [ ] modes without confirmations --force

# features

0. internal file list
	- [x] filter time (start/end date) "--after $date" "--before $date
	- [x] filter file name regex "--match $name-regex"
	- [x] recursive
1. list all files
	- [x] output filenames
2. copy/move files to new directory
	- [x] copy
	- [x] move
	- [x] --force
3. rename files
	- [x] default upa
	- [x] own date-time-format "--format $format"
4. group files
	- [x] upa "--group upa"
	- [x] place-event directories "--group places-events"
6. skrink files for email
	- [ ] downscale "--shrink"
7. reformat (format/quality) files
	- [ ] format "--format $format"
	- [ ] quality "--quality $quality"

Idea
	- file ops return struct{did-something, success}