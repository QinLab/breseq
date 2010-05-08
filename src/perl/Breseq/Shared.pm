###
# Pod Documentation
###

=head1 NAME

Breseq::Shared.pm

=head1 SYNOPSIS

Various utility functions.

=head1 DESCRIPTION

=head1 AUTHOR

Jeffrey Barrick
<jbarrick@msu.edu>

=head1 COPYRIGHT

Copyright 2008.  All rights reserved.

=cut

###
# End Pod Documentation
###

package Breseq::Shared;
use strict;

use Breseq::Fastq;

require Exporter;
our @ISA = qw( Exporter );
our @EXPORT = qw( 	@int_to_base 
					%base_to_int 
					poisson
				);

use Data::Dumper;

##
# Global variables that are exported.
##

our %base_to_int = ( 'A'=>0, 'C'=>1, 'T'=>2, 'G'=>3, '.'=>4, 'N'=>5 );
our @int_to_base = ('A','C','T','G','.','N');

##
# Global variables that are NOT exported.
##	

sub system
{
	my ($command, $silent, $continue) = @_;	
	print STDERR "[system] $command\n" if (!$silent);
	my $res = CORE::system $command;
	die "$res" if ($res && !$continue);
	return $res;
}

sub poisson
{
	my ($x, $lambda) = @_;

	my $pr = exp(-$lambda);

	#divide by factorial
	for (my $i=1; $i<=$x; $i++)
	{
		$pr *= $lambda;
		$pr /= $i;
	}
	
	return $pr;
}

sub tam_next_read_alignments
{
	my ($tam, $header, $last_alignment, $paired) = @_;
	
	my $num_to_slurp = ($paired) ? 2 : 1;
	my $last_read_name;
	my $al_ref;
	if (defined $last_alignment)
	{
		$last_read_name = $last_alignment->qname;
		push @$al_ref, $last_alignment;
		$last_alignment = undef;
	}
	
	
	my $num_slurped = 0;
	ALIGNMENT: while (1)
	{
		
		$last_alignment = Bio::DB::Bam::Alignment->new();
		my $bytes = $tam->read1($header, $last_alignment);
		
		#returns bytes == -1 if EOF reached
		return ($al_ref, undef) if ($bytes < 0);
		
		my $read_name = $last_alignment->qname;
		
		if ((defined $last_read_name) && ($read_name ne $last_read_name))
		{
			$num_slurped++;
			last ALIGNMENT if ($num_slurped == $num_to_slurp);
		}
		
		$last_read_name = $last_alignment->qname if (!defined $last_read_name);
		
		push @$al_ref, $last_alignment;
	}
	
	return ($al_ref, $last_alignment);
}

sub tam_write_read_alignments
{
	my ($fh, $header, $fastq_file_index, $al, $trims) = @_;
#	print Dumper($fh, $header, $fastq_file_index, $al, $trims);
	
	for (my $i=0; $i<scalar @$al; $i++)
	{
		my $a = $al->[$i];
		my $trim = $trims->[$i];
		
		my $aux_tags = 'AS:i:' . $a->aux_get('AS') . "\t" . 'X1:i:' . (scalar @$al) . "\t" . "X2:i:$fastq_file_index";
		$aux_tags .= "\t" . "XL:i:$trim->{L}" . "\t" . "XR:i:$trim->{R}" if (defined $trim);
	
		my @score_array = $a->qscore;
		my $quality_score_string = '';
		foreach my $s (@score_array)
		{
			$quality_score_string .= chr($s+33);
		}
		
		my $cigar_list = $a->cigar_array;
		my $cigar_string = '';
		foreach my $c (@$cigar_list)
		{
			$cigar_string .= $c->[1] . $c->[0];
		}
		
		my @ll;
		push @ll, $a->qname;
		push @ll, $a->flag;
		push @ll, $header->target_name()->[$a->tid];
		push @ll, $a->start;
		push @ll, $a->qual, $cigar_string, ($a->proper_pair ? '=' : '*'), $a->mate_start, $a->isize, $a->qseq, $quality_score_string, $aux_tags;
				
		print $fh join("\t", @ll) . "\n";
	}
}

## We have to project the alignment onto a different reference sequence
## one end marked as a continuation so this can be shown in alignment.

sub tam_write_moved_alignment
{
	my (
		$fh, 					# File handle of TAM file we are writing to
		$a, 					# CJ: SAM alignment object for the read to the candidate junction
		$fastq_file_index, 		# which fastq file this read came from
		$seq_id, 				# REFERENCE: sequence id
		$reference_pos, 		# REFERENCE: position of this junction side
		$reference_strand, 		# REFERENCE: strand of this junction side (-1 or +1)
		$reference_overlap, 	# REFERENCE: amount of overlap in the reference coords
		$junction_side, 		# CJ: side of the junction (1 or 2) that we are writing
		$junction_flanking, 	# CJ: number of bases before overlap in the candidate junction sequence 
		$junction_overlap, 		# CJ: amount of overlap in the candidate junction sequence that we aligned to
		$trim					# CJ: list with two items, indicating what the trim on each end is
	) = @_;

	my $verbose = 0;
	
	if ($verbose)
	{
		print STDERR "qname                 = " . $a->qname . "\n";
		print STDERR "seq_id                = $seq_id\n";
		print STDERR "reference_pos	        = $reference_pos\n";
		print STDERR "reference_strand      = $reference_strand\n";
		print STDERR "reference_overlap     = $reference_overlap\n";
		print STDERR "junction_side         = $junction_side\n";
		print STDERR "junction_flanking     = $junction_flanking\n";
		print STDERR "junction_overlap      = $junction_overlap\n";
		print STDERR "alignment->start      = " . $a->start . "  alignment->end = " . $a->end . "\n";
		print STDERR "trim...\n";
		print STDERR Dumper($trim);
	}

	# Which strand of the read are we on? Controls whether CIGAR is reversed
	my $read_strand = (($junction_side==1) ? -1 : +1) * $reference_strand;
	print STDERR "read strand = $read_strand\n" if ($verbose);

	my ($a_read_start, $a_read_end) = Breseq::Shared::alignment_query_start_end($a);
	print STDERR "a_read_start = $a_read_start, a_read_end = $a_read_end\n" if ($verbose);

	# Setup all of the original read information
	my @qual_scores = $a->qscore;
	my $cigar_list = $a->cigar_array;
	my $seq = $a->qseq;
	my $flags = $a->flag;
	
	## Remove soft padding from CIGAR (since it does not correspond to the
	## aligned positions we are dealing with. It is added back later.
	my $left_padding = 0;
	my $right_padding = 0;
	if ($cigar_list->[0]->[0] eq 'S')
	{
		$left_padding = $cigar_list->[0]->[1];
		shift @$cigar_list; 
	}
	if ($cigar_list->[-1]->[0] eq 'S')
	{
		$right_padding = $cigar_list->[-1]->[1];
		pop @$cigar_list; 
	}
	
	## If we are operating on the opposite read strand,
	## Reverse complement sequence, reverse quals, and toggle strand bit
	if ($read_strand == -1)
	{
		$seq = Breseq::Fastq::revcom($seq);
		@qual_scores = reverse @qual_scores;
		$flags ^= 16; #bitwise XOR to flip strand
	}
	
	
	## $junction_pos gives the position in the CJS 
	## where we want to split the read and only count one side
	## For side 1, we go up to this coordinate
	## Side 2 begins after this coordinate
	my $junction_pos = $junction_flanking;
	if ($junction_side == 1) 
	{
		## offset to include the redundant part for overlaps > 0 on junction_side 1
		$junction_pos += $junction_overlap if ($junction_overlap > 0);
	}
	elsif ($junction_side == 2)
	{
		## Offset past the common part of the alignment overlap:
		##   for negative values, this is a gap
		##   for positive values, this is the common sequence
  		$junction_pos += abs($junction_overlap);

		## Offset backwards for any REMAINING positive overlap.
		## that was not resolved in favor of one side or the other
		## Note that this results in aligning this overlap region
		## TWICE to the reference genome and may not be ideal
		$junction_pos -= $reference_overlap if ($reference_overlap > 0)
	}
	print STDERR "junction_pos = $junction_pos\n" if ($verbose);
	
	###
	## split the CIGAR list into two halves and keep track of their length in the read
	###
	
	print STDERR "Original CIGAR:\n" . Dumper($cigar_list) if ($verbose);
	
	## We want to determine how much of the read matches on each side
	## of this position, use the CIGAR to correct for indels:
	## At the same time, split the CIGAR
		
	my (@side_1_cigar_list, @side_2_cigar_list);
		
	my $test_read_pos = $a_read_start;
	my $test_junction_pos = $a->start;
	
	## it's possible that due to overlap we are already past the position we want
	if ($test_junction_pos > $junction_pos)
	{
		$test_junction_pos = $junction_pos;
	}
	else	
	{
		## Remove CIGAR operations until we have enough length for side 1	
		while (my $c = shift @$cigar_list)
		{
			my ($op, $n) = @$c;
			if ($op eq 'I') #insertion in read relative to reference
			{
				$test_read_pos += $n;
			}
			else
			{
			
				## If we shot past the correct position, backtrack
				my $overshoot = $test_junction_pos + $n - $junction_pos - 1;
				if ($overshoot > 0)
				{
					## reduce $n so that overshoot is removed
					$n -= $overshoot;
					## push back the reduced match length onto the CIGAR
					## so that it can become part of the side 2 match
					unshift @$cigar_list, [$op, $overshoot];
				}
				## After $n may have been adjusted add it to both positions
				$test_junction_pos += $n;			
				$test_read_pos += $n if ($op ne 'D'); #if not deletion in read relative to reference
			}
		
			push @side_1_cigar_list, [$op, $n];
			last if ($test_junction_pos >= $junction_pos);
		}
	}	
	
	## Use the remaining CIGAR operations to construct side 2
	@side_2_cigar_list = @$cigar_list;
	
	print "test_read_pos = $test_read_pos\n" if ($verbose);
	print "test_junction_pos = $test_junction_pos\n" if ($verbose);
	
	# Determine the matched length on each side of the junction
	#  In the read:
	my $total_read_match_length = $a_read_end - $a_read_start + 1;	
	my $side_1_read_match_length = $test_read_pos - $a_read_start;
	my $side_2_read_match_length = $total_read_match_length - $side_1_read_match_length;
	my $read_match_length = ($junction_side == 1) ? $side_1_read_match_length : $side_2_read_match_length;
	print STDERR "total_read_match_length = $total_read_match_length\n" if ($verbose);
	print STDERR "side_1_read_match_length = $side_1_read_match_length\n" if ($verbose);
	print STDERR "side_2_read_match_length = $side_2_read_match_length\n" if ($verbose);
	print STDERR "read_match_length = $read_match_length\n" if ($verbose);
	
	#  In the candidate junction:
	my $total_junction_match_length = $a->end - $a->start + 1;	
	my $side_1_junction_match_length = $test_junction_pos - $a->start;
	$side_1_junction_match_length = 0 if ($side_1_junction_match_length < 0);
	my $side_2_junction_match_length = $total_junction_match_length - $side_1_junction_match_length;
	my $junction_match_length = ($junction_side == 1) ? $side_1_junction_match_length : $side_2_junction_match_length;
	print STDERR "total_junction_match_length = $total_junction_match_length\n" if ($verbose);
	print STDERR "side_1_junction_match_length = $side_1_junction_match_length\n" if ($verbose);
	print STDERR "side_2_junction_match_length = $side_2_junction_match_length\n" if ($verbose);
	print STDERR "junction_match_length = $junction_match_length\n" if ($verbose);

	# we could still be short of the junction
	my $short_of_junction;
	if ($junction_side == 1)
	{
		$short_of_junction =  $a->start + $total_junction_match_length - 1 - $junction_pos; 
		#we end short of the junction if < 0, so we have to offset the reference position by this
		$short_of_junction = 0 if ($short_of_junction > 0);
	}
	# or started matching after the junction
	else #($junction_side = 2)
	{
		$short_of_junction =  $a->start - $junction_pos - 1;
		#we start past the junction if > 0, so we have to offset the reference position by this
		$short_of_junction = 0 if ($short_of_junction < 0);		
	}
	print STDERR "Short of junction = $short_of_junction\n" if ($verbose);

	# Lots of debug output to make sure the CIGAR list is proper...
	print STDERR "CIGAR for each side:\n" . Dumper(\@side_1_cigar_list, \@side_2_cigar_list) if ($verbose);
	## get the right side of the junction	
	@$cigar_list = ($junction_side == 1) ? @side_1_cigar_list : @side_2_cigar_list;
	print STDERR "CIGAR for this junction side:\n" .Dumper($cigar_list) if ($verbose);
		
	# Add original padding to one end and padding to the other side representing
	# the piece that was not used (is aligned to the other side of the junction)
	print STDERR "Left Padding = $left_padding, Right Padding = $right_padding\n" if ($verbose);
	
	#additional padding on the end that is blocked	
	$left_padding += $side_1_read_match_length if ($junction_side == 2);
	$right_padding += $side_2_read_match_length if ($junction_side == 1);
	
	print STDERR "Adjusted Left Padding = $left_padding, Adjusted Right Padding = $right_padding\n" if ($verbose);

	unshift @$cigar_list, ['S', $left_padding] if ($left_padding);
	push @$cigar_list, ['S', $right_padding] if ($right_padding);

	@$cigar_list = reverse @$cigar_list if ($read_strand == -1);

	print STDERR "Final CIGAR:\n" . Dumper($cigar_list) if ($verbose);


	### It's possible there may not actually be any match on this side
	###  in cases of overlap. We must bail or get negative values
	###  in the resulting CIGAR string.
#	return if (($junction_side == 1) && ($side_1_ref_match_length < 0));
#	return if (($junction_side == 2) && ($side_2_ref_match_length < 0));

	
	## Determine the reference coordinate we will write out for this junction side match.
	## Recall:
	##  strand == 1 means this is the lowest coordinate of that junction side sequence
	##  strand == 0 means this is the highest coordinate	
	my $reference_match_start = ($reference_strand == 1) ? $reference_pos + $short_of_junction : $reference_pos - ($junction_match_length - 1) - $short_of_junction;
	
	####
	#### Convert the CIGAR list back to a CIGAR string
	####
	#### at the same time check to make sure the length
	#### is corect and that there are no negative nums
	my $cigar_string = '';
	my $cigar_length = 0;
	foreach my $c (@$cigar_list)
	{
		die if ($c->[1] <= 0);
		$cigar_string .= $c->[1] . $c->[0];
		$cigar_length += $c->[1] if ($c->[0] ne 'D');
	}
	
	####
	#### Assemble the quality score string
	####
	my $quality_score_string = '';
	foreach my $s (@qual_scores)
	{
		$quality_score_string .= chr($s+33);
	}

	####
	#### Setup custom aux tags
	####
	my $aux_tags = 'AS:i:' . $a->aux_get('AS') . "\t" . 'X1:i:' . '1' . "\t" . "X2:i:$fastq_file_index";
	$aux_tags .= "\t" . "XJ:i:1"; #this flag indicates this is a junction match (could also add which direction?)

	##handle putting the trims in the right places
	##need to be aware if read is trimmed out of existence??
	if (defined $trim)
	{
		my $trim_left = ($junction_side == 1) ? $trim->{L} : 0;
		my $trim_right = ($junction_side == 1) ? 0 : $trim->{R};
		($trim_left, $trim_right) = ($trim_right, $trim_left) if ($read_strand == -1);
		$aux_tags .= "\t" . "XL:i:$trim_left" . "\t" . "XR:i:$trim_right";
	}

	####
	#### Create the TAM line and print
	####
	my @ll;
	push @ll, $a->qname . "-M" . $junction_side;
	push @ll, $flags;
	push @ll, $seq_id;
	push @ll, $reference_match_start;
	push @ll, $a->qual, $cigar_string, ($a->proper_pair ? '=' : '*'), $a->mate_start, $a->isize, $seq, $quality_score_string, $aux_tags;	
	my $l = join("\t", @ll) . "\n";
	print STDERR $l if ($verbose);

	die "CIGAR length does not match calculated length" if ($cigar_length != $a->l_qseq);

	print $fh $l;	
}


sub alignment_query_start_end
{
	my ($a, $options) = @_;
	my $ca = $a->cigar_array;
	
	## The cigar array will be undefined if there was no match for this sequence
	return (0,0) if (!defined $ca || (scalar @$ca < 1));
	
	my $start = 1;
	$start += $ca->[0]->[1] if ($ca->[0]->[0] eq 'S');
	my $end = $a->query->length;
	$end -= $ca->[-1]->[1] if ($ca->[-1]->[0] eq 'S');
	
	if ($a->reversed && !$options->{no_reverse})
	{
		($start, $end) = ($a->query->length - $start + 1, $a->query->length - $end + 1);
		($start, $end) = ($end, $start);
	}
	
	return ($start, $end);
}

our $junction_name_separator = '__';
sub junction_name_join
{
	my $expected_items_1 = 10;
	my $expected_items_2 = 12;
	
	if ( (scalar @_ != $expected_items_1) && (scalar @_ != $expected_items_2) )
	{
		die "Incorrect number of items for junction name.\n" . join(",", @_) . "\nExpected $expected_items_1 or $expected_items_2 items.\n";
	}
	return join "$junction_name_separator", @_;
}
sub junction_name_split
{
	my @s = split /$junction_name_separator/, $_[0];
	my $item;

	$item->{interval_1}->{seq_id} 		= $s[0];
	$item->{interval_1}->{start} 		= $s[1];
	$item->{interval_1}->{end} 			= $s[1];
	$item->{interval_1}->{strand} 		= $s[2];
	$item->{interval_1}->{strand} = -1 if ($item->{interval_1}->{strand} == 0);

	$item->{interval_2}->{seq_id} 		= $s[3];
	$item->{interval_2}->{start} 		= $s[4];
	$item->{interval_2}->{end} 			= $s[4];
	$item->{interval_2}->{strand} 		= $s[5];
	$item->{interval_2}->{strand} = -1 if ($item->{interval_2}->{strand} == 0);

	$item->{overlap} 					= $s[6];
	$item->{unique_read_string} 		= $s[7];

	$item->{flanking_left} 				= $s[8];
	$item->{flanking_right} 			= $s[9];
	
	#redundant items are last, becuse they are created at the last minute
	$item->{interval_1}->{redundant} 	= $s[10];
	$item->{interval_2}->{redundant} 	= $s[11];	
	
	return $item;
}


##UNUSED
## moved here from ReferenceSequence.pm to avoid BioPerl requirement.
sub process_reference_sequences
{
	my ($settings, $summary) = @_;
	$summary->{sequence_conversion}->{total_reference_sequence_length} = 0;
	my $ref_seq_info;
	my $i = 0;
	my $s;
	
	# get two pieces of information from $settings
	my $reference_fasta_file_name = $settings->file_name('reference_fasta_file_name');
	my @genbank_file_names = $settings->file_name('reference_genbank_file_names'); 

	my @junction_only_genbank_file_names = $settings->file_name('junction_only_reference_genbank_file_names'); 
	my %junction_only_hash;
	foreach my $jo (@junction_only_genbank_file_names)
	{
		$junction_only_hash{$jo} = 1;
	}
	
	print STDERR "Loading reference sequences...\n";

	##open fasta file
	open FASTA, ">$reference_fasta_file_name";

	my %loaded_seq_ids;

	foreach my $genbank_file_name (@genbank_file_names, @junction_only_genbank_file_names)
	{
		print STDERR "  Loading File::$genbank_file_name\n";
		
		my $junction_only = $junction_only_hash{$genbank_file_name};
		
		## open this GenBank file		
		open GENBANK, "<$genbank_file_name";
		
		my $seq_id;
		my $ref_seq = '';
		my $specified_length = -1;
		my $actual_length = 0;
		my $seq_definition;
		my $seq_version;
		my $started_on_sequence = 0;
		while ($_ = <GENBANK>)
		{			
			chomp $_;
			
			##end of a record
			if ($_ =~ m/^\s*\/\/\s*$/)
			{				
				## error checking
				($actual_length == $specified_length) or die "Error reading GenBank file entry: $seq_id\nLength in header ($specified_length) does not match length of sequence ($actual_length).\n";  
				(!$loaded_seq_ids{$seq_id}) or die "Duplicate GenBank file entry: $seq_id\n";  
				$loaded_seq_ids{$seq_id}++;
				
				$s->{$seq_id}->{length} = $actual_length;
				$s->{$seq_id}->{definition} = (defined $seq_definition) ? $seq_definition : '';
				
				$s->{$seq_id}->{seq_id} = $seq_id;
				$s->{$seq_id}->{version} = $seq_version;
				
				$s->{$seq_id}->{string} = $seq_id;
				
				
				$summary->{sequence_conversion}->{total_reference_sequence_length} += $actual_length;
				
				## it would be nice to get rid of storing the whole genome in memory
				$ref_seq_info->{ref_strings}->{$seq_id} = $ref_seq;
				$ref_seq_info->{seq_order}->{$seq_id} = $i++;
				
				if (!$junction_only)
				{
					push @{$ref_seq_info->{seq_ids}}, $seq_id;
				}
				else
				{
					push @{$ref_seq_info->{junction_only_seq_ids}}, $seq_id;
				}

				### re-initialize to nothing for next sequence
				undef $seq_id;
				undef $seq_definition;
				undef $seq_version;
				$ref_seq = '';
				$specified_length = -1;
				$started_on_sequence = 0;
				
			}
			elsif ($_ =~ m/\s*LOCUS\s+(\S+)\s+(\d+)\s+bp/)
			{				
				$seq_id = $1; 
				$specified_length = $2;
			}
			elsif ($_ =~ m/\s*DEFINITION\s+(.+)$/)
			{
				$seq_definition = $1; 
			}
			elsif ($_ =~ m/\s*VERSION\s+(.+)$/)
			{
				$seq_version = $1; 
			}			
			elsif ($_ =~ m/\s*ORIGIN/)
			{
				print FASTA ">$seq_id\n";
				$started_on_sequence = 1; 
			}		
			elsif ($started_on_sequence)
			{
				$_ =~ s/(\d|\s)//g;
				$actual_length += length $_;
				print FASTA "\U$_\n"; 
				$ref_seq .= "\U$_";
			}			
		}
	}
	
	## create SAM faidx
	Breseq::Shared::system("samtools faidx $reference_fasta_file_name", 1);
	
	$summary->{sequence_conversion}->{reference_sequences} = $s;
		
	return $ref_seq_info;
}



return 1;

