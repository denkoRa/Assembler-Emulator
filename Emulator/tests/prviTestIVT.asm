.extern main
.text.rutine

resetInterrupt:
	ldc r15, main
	call r15, 0
	movs PC, LR

timerInterrupt:
	ldc r14, 8192
	ldc r15, 46
	out r15, r14
	movs PC, LR


illegalInstruction:
	movs PC, LR

keyboardInterrupt:
	ldcl r12, 1
	shl r12, r12, 9
	ldc r15, 4112
	out r12, r15
	movs PC, LR

.data.IVT
	.long resetInterrupt
	.long timerInterrupt
	.long illegalInstruction
	.long keyboardInterrupt
	.skip 48

.end
