#!/usr/bin/perl
# LOST LINKS - set single tiles in over.txt / under.txt by world position:
#   perl setmap.pl FILE X,Y,CHAR [X,Y,CHAR ...]   (CHAR 'q' for a quote)
use strict; use warnings;
my ($file, @edits) = @ARGV;
open my $in, '<', $file or die "$file: $!";
my @lines = <$in>; close $in;
my @rows; for my $i (0..$#lines) { push @rows, $i unless $lines[$i] =~ /^;/ || $lines[$i] =~ /^\s*$/; }
for my $e (@edits) {
  my ($x, $y, $c) = split /,/, $e, 3;
  $c = "'" if $c eq 'quote';
  my $li = $rows[$y]; my $col = $x + int($x / 20);
  my $old = substr($lines[$li], $col, 1);
  die "($x,$y) is a separator\n" if $old eq '|';
  substr($lines[$li], $col, 1) = $c;
}
open my $out, '>', $file or die; print $out @lines; close $out;
