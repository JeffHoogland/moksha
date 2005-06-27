#!/usr/bin/perl

my $TODO;

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

my $item_count;
my $total_item_count = 0;

while(<>) {
	chomp;    

	if(/\[\[\[/) {
		$in_body = 1;
	} elsif (/\]\]\]/) {
		$in_body = 0;
	} elsif (/^---.*---$/) {
		if ($in_body) {
			if($is_title) {
				$is_title = 0;
			} else {
				$is_title = 1;
			}
		}
	} elsif (/^\* .*/) {
		if ($in_body ) {
			$total_item_count++;
			$_ =~ s/^\* //;
			push(@{$todo_hash{$title}}, $item);
			$item = {};
			$item->{'asignee'} = "Unknown";
			$item->{'task'} = $_ ;
			if(/<(.*)>/) {
				$item->{'asignee'} = $1;
			}
		}
	} else {
		if ($in_body) {
			if($is_title) {
				$title = $_;
			} else {
				$item->{'task'} = $item->{'task'} . $_ ;
				if(/<(.*)>/) {
					$item->{'asignee'} = $1;
				}
			}
		}
	}

}

for $title ( keys %todo_hash ) {

	print "<h2>" . $title . "</h2>\n";

	print "<table>\n";
	print "    <tr><td>Asignee</td><td>Task</td></tr>\n";
	for $item ( @{$todo_hash{$title}} ) {
		my $asignee = $item->{'asignee'};
		my $task = $item->{'task'};
		print "    <tr><td>$asignee</td><td>$task</td></tr>\n";
	}
	print "</table>\n";
}
