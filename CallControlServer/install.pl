#!/usr/bin/perl

use File::Copy;

$dir = "/home/zhangjun/projects/alpha/CallControlServer";

$oldrepro = "$dir/bin.debug.Linux.i686/repro";
$newrepro = "$dir/repro/bin.debug.Linux.i686/repro";

$olddum = "$dir/bin.debug.Linux.i686/libdum.so";
$newdum = "$dir/resip/dum/obj.debug.Linux.i686/libdum.so";

$oldstack = "$dir/bin.debug.Linux.i686/libresip.so";
$newstack = "$dir/resip/stack/obj.debug.Linux.i686/libresip.so";

$oldrutil = "$dir/bin.debug.Linux.i686/librutil.so";
$newrutil = "$dir/rutil/obj.debug.Linux.i686/librutil.so";

$oldb2bua = "$dir/bin.debug.Linux.i686/libb2bua.so";
$newb2bua = "$dir/b2bua/obj.debug.Linux.i686/libb2bua.so";

if (-M $newrepro < -M $oldrepro)
{
    warn "repro updated!\n" ;
    print -M $oldrepro, ' | ', -M $newrepro, "\n";
    print "copy $newrepro, $oldrepro \n";
    copy( $newrepro, $oldrepro )
	or die "copy failed: $!";
}

if ( -M $newdum < -M $olddum )
{
    warn "dum updated!\n";
    print -M $olddum, '|', -M $newdum, "\n";
    print "copy $newdum, $olddum \n";
    copy( $newdum, $olddum )
        or die "copy failed: $!";
}

if ( -M $newstack < -M $oldstack )
{
    warn "stack updated!\n";
    print -M $oldstack, '|', -M $newstack, "\n";
    print "copy $newstack, $oldstack \n";
    copy( $newstack, $oldstack )
        or die "copy failed: $!";
}

if ( -M $newrutil < -M $oldrutil )
{
    warn "rutil updated!\n";
    print -M $oldrutil, '|', -M $newrutil, "\n";
    print "copy $newrutil, $oldrutil \n";
    copy( $newrutil, $oldrutil )
        or die "copy failed: $!";
}


if ( -M $newb2bua < -M $oldb2bua )
{
    warn "b2bua updated!\n";
    print -M $oldb2bua, '|', -M $newb2bua, "\n";
    print "copy $newb2bua, $oldb2bua \n";
    copy( $newb2bua, $oldb2bua )
        or die "copy failed: $!";
}




