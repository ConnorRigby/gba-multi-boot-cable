#!/usr/bin/perl

print "unsigned char loader[] = { ";

$len = 0;

$l = 1;

while ($l != 0)
{
    my $i;
    $l = sysread(STDIN, $i, 1);
    if ( $l == 1)
    {
	printf("0x%x,", ord($i));
	$len++;
    }
}

print " 0 };\n";
print "int loaderlen = $len;\n";
