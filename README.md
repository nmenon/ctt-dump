ctt-dump
========

Texas Instrument's Clock Tree Tool(CTT) is an interesting visualization tool for
various internal clock tree instances.

http://omappedia.org/wiki/CTT has more details on how to use the tool.

One of the best debug facility is the ability of CTT to read-in register values
to visualize Clock tree for most available TI platforms. This tends to be the
same as the read-out of registers.

To see the platforms for which CTT is available, see:
http://focus.ti.com/general/docs/wtbu/wtbudocumentcenter.tsp?templateId=6123&navigationId=12667

Obviously, due to licensing constraints, we may not include the appropriate platform's CTT
in this repository.

HOWTO BUILD
==========
* make - to build a binary
* make clean - to cleanup
* make sandbox - testing binary build (no register access done)

**NOTE: Also depends on 'CROSS_COMPILE' variable for compiler choice.**

HOWTO USE
=========
However, a simple utility provided by ctt-dump is used as follows for any of the supported platforms:
* download CTT tool appropriate for your platform
* startup CTT and generate a read-out (menu-> Register Dump -> Dump-out) ->
   This generates, what we call a rd1 file - example: use test.rd1
* copy the test.rd1 file to the target platform
* Run:
```
ctt-dump -i test.rd1 -o output.rd1
```
This generates an output.rd1 which can then be imported to CTT.

OR
```
ctt-dump -i test.rd1
```
This outputs to console the rd1 output file which could be copied into a file for import into CTT.
* In CTT, (menu-> Register Dump -> Read-In) - the output from ctt-dump  and once you setup the sysclks needed, you should ideally have the appropriate clock tree for visualization

WARNING
=======
* ctt-dump is a userspace utility - there are no hooks to control clock mutexes, or other OS specific details. So if the register configurations change while the utility runs, there is nothing much one can do.
* CTT data **ASSUMES** that the CTT is mature enough, remember that CTT for certain platforms may be hand generated and could have errorneous data. - in such cases, please provide feedback to CTT, this tool tries to remain simple.

HOWTO CONTRIBUTE
===============
* just send me github pull requests/use the github portal to file defects.
