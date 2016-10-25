.global main

.text.Main


	
main:	
	ldc r6, busyWait
	ldcl r13, 4112
	add r0, 1
	shl PSW, r0, 30
busyWait:
	in r12, r13
	shr r12, r12, 9
	subs r12, 1
	movne PC, r6	

	ldc r4, 8192
	ldc r5, 4096

	in r8, r5

	ldc r12, 0
	out r12, r13

	add r8, 1
	out r8, r4
	
	mov PC, r6

.end 
