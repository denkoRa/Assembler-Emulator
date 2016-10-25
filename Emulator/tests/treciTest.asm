.global main
.extern fact

.bss.storage
	.skip 16
freeSpace:
	.skip 200

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
	
	ldcl r9, 1
	ldcl r15, 4096
	in r8, r15
	
	ldc r12, 0
	out r12, r13
	
	sub r8, 48
	ldc r1, fact
	ldc r7, continue
	ldcl r2, 0
	cmps r8, r2
	callgt r1, 0
	

continue:
	ldc r10, print
	ldc r11, freeSpace
	ldc r12, computeDigits
	mov PC, r12

computeDigits:
	
	cmps r9, 0
	ldcleq r2, 10
	streq r2, [r11++]
	moveq PC, r10

	mov r1, r9
	div r1, 10
	mul r1, 10
	
	mov r2, r9
	sub r2, r1
	add r2, 48
	
	str r2, [r11++]

	div r9, 10

	mov PC, r12


print:
	ldc r2, freeSpace
	sub r2, 4
	cmps r2, r11
	ldcleq r12, 0
	ldcleq r2, 10
	outeq r2, r4
	moveq PC, r6

	
	ldr r3, [--r11]
	out r3, r4

	mov PC, r10
			
	
.end

