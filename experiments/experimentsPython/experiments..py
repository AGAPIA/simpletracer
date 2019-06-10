from pwn import *

bogdan = process("/home/ceachi/testtools/simpletracer/experiments/testingExecution/main")

input = bogdan.recv()
print(input)
river = process("/home/ceachi/testtools/simpletracer/river.tracer/river.tracer -p libfmi.so --annotated --z3 < ~/testtools/river/benchmarking-payload/fmi/sampleinput.txt")
rezultat = river.recvuntil("Rezulat")
print(rezultat)

#http://docs.pwntools.com/en/stable/tubes/processes.html
