#!/usr/bin/perl
# LOST LINKS - replace whole 20-tile zone segments in over.txt / under.txt:
#   perl setseg.pl FILE ZX Y SEGMENT [ZX Y SEGMENT ...]
# ZX is the zone column (0-7), Y the world row (0-71), SEGMENT 20 tiles.
use strict; use warnings;
my ($file, @a) = @ARGV;
open my $in, '<', $file or die "$file: $!";
my @lines = <$in>; close $in;
my @rows; for my $i (0..$#lines) { push @rows, $i unless $lines[$i] =~ /^;/ || $lines[$i] =~ /^\s*$/; }
while (@a) {
  my ($zx, $y, $seg) = splice @a, 0, 3;
  die "segment for zone $zx row $y is ".length($seg)." wide\n" unless length($seg) == 20;
  my $li = $rows[$y];
  substr($lines[$li], $zx * 21, 20) = $seg;
}
open my $out, '>', $file or die; print $out @lines; close $out;
