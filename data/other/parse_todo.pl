#!/usr/bin/perl

use strict;

my $new_item = 0;
my $in_body  = 0;
my $is_title = 0;

# this hash is like this
#
# hash<str, arr<hash<str,str>>>
#
# [title1] -> [ (item1, asignee1), (item2, asignee2) ]
# [title2] -> [ (item1, asignee1) ]
#
my %todo_hash;
my $title;
my $item = {};

# use globals
sub push_item {
    	if ($item->{'task'}) {
		my $task = $item->{'task'};
		$task =~ s/^\* //;
		if ($task =~ s/<(.*)>//) {
			$item->{'asignee_email'} = $1;
			$1 =~ /(.*) AT /;
			$item->{'asignee'} = $1;
		} else {
			$item->{'asignee_email'} = 0;
			$item->{'asignee'} = 'None';
		}
		$item->{'task'} = $task;
		push(@{$todo_hash{$title}}, $item);
	}
}

while(<>) {
	chomp;    

	if(/\[\[\[/) {
		$item->{'task'} = 0;
		$in_body = 1;
	} elsif (/\]\]\]/) {
		if ($in_body) {
			push_item;
			$item = {};
			$in_body = 0;
		}
	} elsif (/^---.*---$/) {
		if ($in_body) {
			if($is_title) {
				$is_title = 0;
			} else {
				$is_title = 1;
			}
		}
	} elsif (/^\* /) {
		if ($in_body ) {
			push_item;
			$item = {};
			$item->{'task'} = $_ ;
		}
	} else {
		if ($in_body) {
			if ($is_title) {
				$title = $_;
			} else {
				$item->{'task'} = $item->{'task'} . $_ ;
			}
		}
	}

}

for $title ( keys %todo_hash ) {
	my $count = 1;
	print "<h2>" . $title . "</h2>\n";

	print "<table>\n";
	print "    <tr><td>#</td><td>Asignee</td><td>Task</td></tr>\n";
	for $item ( @{$todo_hash{$title}} ) {
		my $asignee_email = $item->{'asignee_email'};
		my $asignee = $item->{'asignee'};
		my $task = $item->{'task'};

		my $mailto;

		if ($asignee_email) {
		     $mailto = "<a href='mailto://$asignee_email'>$asignee</a>"	
		} else {
		     $mailto = $asignee;
		}
		print "    <tr><td>$count</td><td>$mailto</td><td>$task</td></tr>\n";
		$count++;
	}
	print "</table>\n";
}
