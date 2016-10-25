.global main

.bss.storage
	.skip 8
freeSpace:
	.skip 80
.text.Main


	
main:	
	ldc r6, busyWait
	ldcl r13, 4112
	ldc r11, readDigit
busyWait:
	in r12, r13
	shr r12, r12, 9
	subs r12, 1
	movne PC, r6	

	ldc r5, 4096
	in r8, r5
	
	cmps r8, 10
	addeq r0, 1
	movne PC, r11
	cmps r0, 2
	ldceq r3, gcd
	ldceq r4, continue
	moveq PC, r3
	movne PC, r6

readDigit:
	sub r8, 48
	

	cmps r0, 0
	muleq r1, 10	
	addeq r1, r8
	mulne r2, 10
	addne r2, r8
	
	mov PC, r6


gcd:
	cmps r1, r2
	moveq PC, r4
	subgt r1, r2
	sublt r2, r1
	mov PC, r3

continue:
	mov r9, r1
	ldc r10, print
	ldc r4, computeDigits
	ldc r12, freeSpace
	
computeDigits:
	
	cmps r9, 0
	
	ldceq r4, 8192
	moveq PC, r10

	mov r1, r9
	div r1, 10
	mul r1, 10
	
	mov r2, r9
	sub r2, r1
	add r2, 48
	
	str r2, [r12++]

	div r9, 10

	mov PC, r4


print:
	ldc r2, freeSpace
	
	cmps r2, r12

	ldcleq r2, 10
	outeq r2, r4
	ldceq r0, 0
	ldceq r1, 0
	ldceq r2, 0
	moveq PC, r6

	
	ldr r3, [--r12]
	out r3, r4

	mov PC, r10
			
	
.end



