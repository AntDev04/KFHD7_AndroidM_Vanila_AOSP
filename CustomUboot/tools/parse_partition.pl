#!/usr/bin/perl
###################################################
#
#	parse_partition.pl
#
#	Usage: parse_partition [InputFile] [OutputFile] [OutputShFile]
#
#	BokeeLi, 2011/10/03
###################################################


###################################################
##            Setting Variable
###################################################

$numArgs = $#ARGV + 1;

my $INPUT_FILE = $ARGV[0];
my $OUTPUT_FILE = $ARGV[1];
my $OUTPUT_SH_FILE = $ARGV[2];
my $IS_VALID = 1;
my $OFFSET = 0;
my $PARTITION_NAME;
my $PARTITION_SIZE = 0;
my $line;

use Cwd;
my $cwd = getcwd();

#use strict;
#use warnings;

############################
##      Main Program      ##
############################
OpenFiles();

# try to find the partition table
# static struct partition partitions[] = { 
# .... 
# }
while ($line=<InFd>)
{
	# Handle #if 0 or #if 1 extra
	if($line =~/#if 0/)
	{
		$IS_VALID = 0;	
	}
	if($line =~/#if 1/)
	{	
		$IS_VALID = 1;	
	}
	if($line =~/#else/)
	{
		if($IS_VALID == 1)
		{
			$IS_VALID = 0;			
		}
		elsif ($IS_VALID == 0)
		{
			$IS_VALID = 1;
		}
	}
	if($line =~/#endif/)
	{
		$IS_VALID = 1;
	}

	if($line =~ /static struct partition partitions\[\]/i && $IS_VALID == 1)
	{
		last;
	}
}
TransPartition();
CloseFiles();

# ------------------------------------------------------------------------
# SUBROUTINE: open_files 
#
#   Open all files
# 
# return:
#   None
#
# ------------------------------------------------------------------------
sub OpenFiles
{
	if(-e $INPUT_FILE && length($OUTPUT_FILE))
	{
		open(OutFd,"+>$OUTPUT_FILE");
		open(InFd,"<$INPUT_FILE");
		open(OutShFd, "+>$OUTPUT_SH_FILE");
	}
	else
	{
		my $str= "Usage: ParsePartition [InputFile] [OutputFile] [OutputShFile]\n".	  
			"To generate partition cfg file\n".
			"[InputFile] should be .c or .h\n".
			"[OutputFile] should be .cfg\n".
			"[OutputShFile] should be .sh\n";
		die($str);
	}
}

				
# ------------------------------------------------------------------------
# SUBROUTINE: CloseFiles 
#
#   Close all files
#
# return:
#   None
#
# ------------------------------------------------------------------------
sub CloseFiles
{
	close(OutFd);
	close(OutShFd);
	close(InFd);
}


# ------------------------------------------------------------------------
# SUBROUTINE: TransPartition 
#
#   Parse partition table and write into cfg and script file
#
# return:
#   None
#
# ------------------------------------------------------------------------

sub TransPartition
{
	while ($line=<InFd>)
	{
		# x: ignore blank
		if($line =~ /"{1}.*"{1},{1}.*\d+.*,{1}/x)  # Result such as: "xxx", abc*def }, 
		{
			#printf "Match: $&\n";
			#printf "Before: $`\n";
			#printf "After: $'\n";

			my $MatchedString = $&;
			my $PreString;
			#printf "MatchedString:$MatchedString\n";
			
			if($MatchedString =~/},/x)  # result such as: "xxx", abc*def
			{
				$PreString = $`;
				#printf "PreString: $PreString\n";
			}
			
			# Do not ignore blank
			if($PreString =~/, /)  # Result such as: "xxx" abc*def
			{
				my $PARTITION_SIZE_TEMP=$';
				my $PARTITION_NAME_TEMP=$`;
				#printf "PARTITION_SIZE_TEMP:$PARTITION_SIZE_TEMP, PARTITION_NAME_TEMP:$PARTITION_NAME_TEMP\n";
				
				if($PARTITION_NAME_TEMP =~/[a-zA-Z0-9_-]+/x)
				{
					$PARTITION_NAME=$&;
				}
				
				if($PARTITION_SIZE_TEMP =~/\*/x)
				{
					$PARTITION_SIZE=$'* $` * 1024;
				}
				else
				{
					$PARTITION_SIZE=$PARTITION_SIZE_TEMP * 1024;
				}

				printf OutFd "[partition]\n";
				printf OutFd "name=$PARTITION_NAME\n";
				printf OutFd "offset=$OFFSET\n";
				#printf OutFd "offset=0x%lx\n", $OFFSET;
				printf OutFd "size=$PARTITION_SIZE\n";
				#printf OutFd "size=0x%lx\n", $PARTITION_SIZE;
				printf OutFd "\n";

				# If partition name is "-" using string comparison
				if($PARTITION_NAME eq "-")
				{
					printf OutShFd "export EMPTY_SIZE=%d\n", $PARTITION_SIZE;
				}
				else
				{
					# Transform to uppercase
					printf OutShFd "export %s_SIZE=%d\n", uc($PARTITION_NAME), $PARTITION_SIZE;
				}

				$OFFSET += $PARTITION_SIZE;
				#printf "PARTITION_NAME: $PARTITION_NAME, PARTITION_SIZE: $PARTITION_SIZE\n";
			}
		}
		elsif($line =~/;/)
		{
			last;
		}
	}
}


