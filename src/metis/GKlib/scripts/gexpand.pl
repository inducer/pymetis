#!/usr/bin/perl -w

die "Usage $0 <gfile> <ncopies>\n" unless @ARGV == 2;

$filein  = shift(@ARGV);
$ncopies = shift(@ARGV);

open(FPIN, "<$filein") or die "Could not open $filein. $!\n";

$_ = <FPIN>;
chomp($_);
($nvtxs, $nedges) = split(' ', $_);

#print "nvtxs: $nvtxs, nedges: $nedges\n";

$u = 1;
while (<FPIN>) {
  chomp($_);
  @edges = split(' ', $_);

  # put the within layer edges
  foreach $v (@edges) {
    next if $v < $u;
    for ($i=0; $i<$ncopies; $i++) {
      printf("%d %d\n", $i*$nvtxs+$u-1, $i*$nvtxs+$v-1);
      printf("%d %d\n", $i*$nvtxs+$v-1, $i*$nvtxs+$u-1);
    }
  }

  # put the vertex across layer edges
  for ($i=0; $i<$ncopies-1; $i++) {
    printf("%d %d\n", $i*$nvtxs+$u-1, ($i+1)*$nvtxs+$u-1);
    printf("%d %d\n", ($i+1)*$nvtxs+$u-1, $i*$nvtxs+$u-1);
  }

  # put the adjacent across layer edges
  for ($i=0; $i<$ncopies-1; $i++) {
    $j=0;
    foreach $v (@edges) {
      $j++;
      next if (($j+$i)%2 == 0);
      printf("%d %d\n", $i*$nvtxs+$u-1, ($i+1)*$nvtxs+$v-1);
      printf("%d %d\n", ($i+1)*$nvtxs+$v-1, $i*$nvtxs+$u-1);
    }
  }

  goto DONE;

DONE:
  $u++;
}

close(FPIN);
