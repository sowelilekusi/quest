Introduction
============
The quest file format is a declarative computer language. It is made up
of statements that when read by the software define how it should behave and
statements that are written by the software to declare what it recorded.
Each statement begins with a directive followed by options or arguments indented
by one tab on the following lines.
The order of the directives is important, they are read by the program
line by line and may inherit data from those placed above.
If a line does not begin with a recognizable directive, it is ignored.
Care should be taken when inserting comments as one program may recognize
directives that another does not, for this reason double backslashes (//) are
reserved for leaving comments

List of directives
==================
The indented section of the directives reference show example values first,
followed by the range of possible values or brief explaination in brackets

Option directives
-----------------
Any client is free to define their own directives to look for in this section.
These directives override a programs defaults, they should be placed at the
top of the file alongside MetaData directives

Foreground-Color
	247, 248, 242   (0-255, 0-255, 0-255)

Background-Color
	47, 53, 66      (0-255, 0-255, 0-255)

Best-Color
	249, 255, 79    (0-255, 0-255, 0-255)

Ahead-Gaining-Color
	24, 240, 31     (0-255, 0-255, 0-255)

Ahead-Losing-Color
	79, 255, 85     (0-255, 0-255, 0-255)

Behind-Gaining-Color
	255, 79, 79     (0-255, 0-255, 0-255)

Behind-Losing-Color
	224, 34, 34     (0-255, 0-255, 0-255)

Comparison
	Personal-Best   (Personal-Best/World-Record/Sum-of-Best)

Show-Delta-Column
	True            (True/False)

Show-Segment-Column
	True            (True/False)

Show-Time-Column
	True            (True/False)

Show-Comparison-Column
	True            (True/False)

Toggle-Hotkeys
	t               (See key name section)

Start-Key
	r               (See key name section)

Stop-Key
	f               (See key name section)

Pause-Key
	y               (See key name section)

Split-Key
	e               (See key name section)

Unsplit-Key
	g               (See key name section)

Skip-Key
	v               (See key name section)

MetaData directives
---------------
These directives provide metadata to runs that follow them,
multiple of these can appear in a single file if multiple
games or categories are run after one another

Title
	Elden Ring      (Title of the game being run)

Category
	Any%            (Name of the category being run)

Runner
	SuperCoolGuy04  (The name you as the runner are known by)

Segment and Route Directives
----------------------------
Runs by themselves are simply a list of events that occured
(such as splitting or pausing) in the quest language, in order to give meaning
to this data, these more complicated directives are used to define segments
that are played between splits and routes made up of these segments.
Define all your possible segments first, followed by all routes.
If no segments are defined, a single unnamed segment is to be assumed.
If no routes are defined, a single unnamed route that passes through all
segments in the order of their definition is to be assumed.

Segment
	Shortname
		Stage One       (A name for the segment that should identify it
	                 	 within the file and refer to the single 
				 objective completed.
				 This name should not change.)
	Longname
		Murderize Chad  (The display name for the segment if it should
	                 	 differ from the proper name, this name can be
				 changed freely and may be used for
				 inside-jokes without issue.)
	Description
		Go to the golden palace and kill
		the big dude with the ugly sunglasses
		                (Optional third argument for use as a more
				 detailed note or reminder of the objective
				 for the segment that may be displayed
				 alongside the name)
	
Route
	Name
		Magic Swordless (Name for the route, keeping in mind routes
		                 are not categories, you may simply run
				 multiple different routes because you're unsure
				 yet which strategies are the fastest)
	Segments
		Stage One       (The ordered list of segments the route
		Stage Two        consists of, these names should match the
		Stage Four       segment Shortnames)

Run Directives
--------------
These directives are much more complicated and are not intended to be written
by a human but rather by the timer software, they will make up the majority
of a file as they are the run history which may be quite long.
The data passed by these directives exists agnostic of segments, route, games,
or categories, rather they are either explicitly matched with metadata that is
applicable, or by default is matched with the last set of metadata declared by
the time of the run directive

Run
	Category
		Any%
	Route
		Magic Swordless
	Start
		2016-10-23 10:03:12.034Z
		                (The first event in a run must be Start, and it
				 must be accompanied by an RFC 3339 timestamp)
	Split
		120052          (Following events may be accompanied by
		                 RFC 3339 timestamps or simply a millisecond
				 offset from the previous event)
	Skip
		323481
	Split
		3121111
	Pause
		421397
	Resume
		2016-10-23 11:16:04.175Z
	Stop                   (The last event in a run is always a Stop)
		123111

The 'Then' Directive
------------------
The default behaviour the timer should exhibit when multiple games, catagories,
or routes exist in the same quest file is to allow the user to select
which one should be run, but the 'Then' directive instead allows for multiple
routes or catagories to be run one straight after the other as a single longer
"route". The timer should insert a dummy "segment" between the two routes for
the user to spend resetting the game and record the resulting run in two places:
A run directive should be recorded with every event including the dummy split
that can be exported and shared as one marathon attempt
AND
Individual run directives for each of the individual routes should be created
without the other included routes or the dummy split that can contribute to the
attempt history and stats of those individual catagories
If multiple routes exist on both or either side of a Then directive, the routes
to be stitched together should be selectable out of the options given, just as
they are normally.
