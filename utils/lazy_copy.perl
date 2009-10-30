#!/usr/bin/perl -w

my $confDisks = "/mnt/xenon01_*";
my $confSelectedDisk = $ENV{"HOME"}."/.lazy_copy.dat";

$| = 1;

my $KiB = 1024;
my $MiB = 1024*1024;
my $GiB = 1024*1024*1024;

my $file = shift @ARGV;
my @file = split(/\//, $file);
my $fname = pop @file;

my $size = -s $file;

my $dest = selectDisk($size);

my $dfile = "$dest/$fname";

print "Backup $fname $size bytes $file to $dfile\n";

my $cmd = "/usr/bin/time /bin/cp -rpv $file $dfile";
#my $cmd = "touch $dfile";
print "Run $cmd\n";
system $cmd;

die "Output file $dfile does not exist: $!\n" if ! -e $dfile;

my $dsize = -s $dfile;

die "Output file $dfile size mismatch: $dsize expected $size\n" unless $dsize == $size;

exit 0;

sub diskOkey
  {
    my $disk = shift @_;
    my $size = shift @_;
    my $sizeKiB = int(($size+$KiB-1)/$KiB);

    my $df = `/bin/df $disk | grep -v Filesystem`;
    my @df = split(/\s+/, $df);
    my $availKiB = $df[3];
    #print "Disk $disk $availKiB $sizeKiB\n";

    # we have enough space
    return 1 if ($sizeKiB + 1024 < $availKiB);

    # no space on this disk
    return 0;
  }

sub selectDisk
  {
    my $size = shift @_;
    
    #print "Looking for $size bytes\n";

    if (-r $confSelectedDisk)
      {
	my $d = `cat $confSelectedDisk`;
	chop $d;
	return $d if (diskOkey($d, $size));
      }
    
    foreach my $d (`/bin/ls -1d $confDisks`)
      {
	chop $d;
	if (diskOkey($d, $size))
	  {
	    system "echo $d > $confSelectedDisk";
	    return $d;
	  }
      }
    
    die "Cannot find a disk to write to!";
  }

#end
