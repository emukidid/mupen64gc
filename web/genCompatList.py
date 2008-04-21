#!/usr/bin/env python

import os, sys

def parseList(lines):
	columns = []
	entries = []
	currentColumn = None
	currentEntry  = None
	
	for line in lines:
		if line[0] == '#': continue
		if line[0] == '<':
			title = line[ 1 : line.rfind('>') ]
			if title.find('=') != -1:
				name,url = title.strip().split('=')
				title = '<a href="'+url+'">'+name+'</a>'
			currentEntry = [ title, { } ]
			entries.append(currentEntry)
		elif currentEntry:
			if line[0] == '[':
				currentColumn = line[ 1 : line.rfind(']') ]
				currentEntry[1][currentColumn] = ''
				
				splitter = currentColumn.find('::')
				if splitter != -1:
					major,minor = currentColumn[:splitter], currentColumn[splitter+2:]
					found = False
					for column in columns:
						if major == column[0]:
							found = True
							column[1].append(minor)
					if not found: columns.append( (major, [minor]) )
				else:
					if not currentColumn in columns: columns.append(currentColumn)
				
			elif currentColumn:
				if line[0] == '!':
					if line[1:].startswith('img'):
						images = line.split()[1:]
						for image in images:
							tag = '<img src="'+image+'" width="64" height="48">'
							tag = '<a href="'+image+'">'+tag+'</a>'
							currentEntry[1][currentColumn] += tag
				else:
					currentEntry[1][currentColumn] += line
	return columns, entries
			

#print 'Content-type: text/html'
print '<html>'
print '<header>'
print '<title>Compatibility List - mupen64gc</title>'
print '</header>'

print '<body>'

print '<h3>Compatibility List</h3>'

# TODO: Parse the given list and write it as a table
columns, entries = parseList( open(sys.argv[1]).readlines() )

# Print out the table heading
print '<table border="1">'
print '<tr>'
print '<th> Title </th>'
for column in columns:
	try:
		major, minors = column
		print '<th colspan="'+str(len(minors))+'">', major, '</th>'
	except:
		print '<th>', column, '</th>'
print '</tr>'
# Print out minor columns (different plugins)
print '<tr>'
print '<th> </th>'
for column in columns:
	try:
		major, minors = column
		for minor in minors:
			print '<th>', minor, '</th>'
	except:
		print '<th> </th>'
print '</tr>'

for entry in entries:
	print '<tr>'
	print '<td><i>', entry[0], '</i></td>'
	
	for column in columns:
		try:
			major,minors = column
			for minor in minors:
				name = major+'::'+minor
				try: print '<td>', entry[1][name], '</td>'
				except: print '<td> </td>'
		except:
			try: print '<td>', entry[1][column], '</td>'
			except KeyError: pass
	
	print '</tr>'

print '</table>'

print '</body>'
print '</html>'
