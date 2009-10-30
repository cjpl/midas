#!/usr/bin/perl -w

$| = 1;

my $KiB = 1024;
my $MiB = 1024*1024;
my $GiB = 1024*1024*1024;

my $file = shift @ARGV;
my @file = split(/\//, $file);
my $fname = pop @file;

my $size = -s $file;

print "Backup $fname $size bytes $file\n";

exit 0;

#end
