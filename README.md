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
	- [ ] --dry-run mode showing consequences
	- [ ] modes without confirmations --force

# features

0. internal file list
	- [ ] filter time (start/end date) "--after $date" "--before $date
	- [ ] filter file name regex "--match $name-regex"
	- [ ] recursive
1. list all files
	- [ ] output filenames
2. copy/move files to new directory
	- [ ] copy
	- [ ] move
	- [ ] --force
3. rename files
	- [ ] default upa
	- [ ] own date-time-format "--format $format"
4. group files
	- [ ] upa "--group upa"
	- [ ] place-event directories "--group places-events"
6. skrink files for email
	- [ ] downscale "--shrink"
7. reformat (format/quality) files
	- [ ] format "--format $format"
	- [ ] quality "--quality $quality"
