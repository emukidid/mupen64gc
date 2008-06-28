#!/usr/bin/perl
# Just wraps scheme code in C code
# ss2c.pl in.ss

use IO::Handle;

# Read the source file
$_ = $ARGV[0];
/(.*)\/(.*)\.ss/;
my $dir = $1; print "$1\n";
my $name = $2; print "$2\n";
if(! $name){ die "Please specific a scheme source file"; }

# Open the scheme source
open(SS, "${ARGV[0]}") || die $!;
# Open the c file as stdout
open(C, ">$dir/$name.c") || die $!;
STDOUT->fdopen(\*C, "w") || die $!;

# Variable declartion
print "/* WARNING: Auto-generated file, do not edit */\n\n";
print "char ${name}_scheme_code[] = \n";
# Copy the file basically
# TODO: 'Minify' the code
while($_ = <SS>){
	s/"/\\"/g;
	s/\n//;
	print "\t\"$_\\n\"\n";
}
# Close up the statement
print ";\n\n";
