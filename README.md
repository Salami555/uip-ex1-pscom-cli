# pscom

0. help
	- [x] supported formats (pscom::sf)
	- [x] version of cli and lib (pscom::vi)
1. qDebug/qInfo/qWarning/qCritical/qFatal (VerbosityHandler)
	- [ ] stderr, stdout
	- [x] "--verbosity"
2. progessbar
	- [ ]
3. warn on lossy operations (replace, change, override, remove)
	- [ ] ask user for confirmation
	- [ ] --dry-run mode showing consequences
	- [ ] modes without confirmations --force

# features

1. list all files "-l --list"
	- [ ] filter time (start/end date) "--after $date" "--before $date
	- [ ] filter file name regex "--match $name-regex"
2. copy/move files to new directory
	- [ ] "--copy"
	- [ ] "--move"
3. rename files
	- [ ] upa "--rename upa"
	- [ ] own date-time-format "--rename $format"
4. group files
	- [ ] upa "--group upa"
	- [ ] place-event directories "--group places-events"
6. skrink files for email
	- [ ] downscale "--shrink"
7. reformat (format/quality) files
	- [ ] format "--format $format"
	- [ ] quality "--quality $quality"
