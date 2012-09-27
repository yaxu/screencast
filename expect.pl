use Expect;

use strict;

# create an Expect object by spawning another process
my $exp = Expect->spawn("/usr/bin/screen", "-x")
    or die "Cannot spawn $!\n";

foreach my $i (0 .. 16) {
    print $exp "\cah";
    sleep(5);
}



# if no longer needed, do a soft_close to nicely shut down the command
$exp->soft_close();
