
.global fact


.text.factoriel

fact:
	mul r9, r8
	sub r8, 1
	cmps r8, 0
	callgt fact
	mov PC, LR

.end
