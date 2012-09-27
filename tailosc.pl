#!/usr/bin/perl -w
use strict;

my $file = shift @ARGV;

my $last = "";
while (1) {
    my $foo = `tail -n 2 $file`;
    if ($foo ne $last) {
	$last = $foo;
	chomp $foo;
	$foo =~ s/\(\d+:\d+:\d+\)//gs;
	use Net::OpenSoundControl::Client;
	my $osc = Net::OpenSoundControl::Client->new(Host => "127.0.0.1",
						     Port => 7777
	    )
	    or die;
	$osc->send(['/text/irc',
		    's', $foo
		   ]);
    }
    sleep(5);
}
