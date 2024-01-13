; Proof of concept: tool to change Compaq Portable III speed to HIGH
; No checks are performed.

; Build:
; tasm sethigh
; tlink sethigh /t

   .MODEL tiny
   .CODE
   
ORG 0100h

start: 
   push    cs
   pop     ds

   mov     ah, 9                   
   mov     dx, OFFSET HelloMessage 
   int     21h                    

   mov     ax, 0F002h
   int     16h
   
   mov     ax, 4C00h      ; DOS terminate program function with 00 exit code.
   int     21h          

HelloMessage DB 'Changing Compaq Portable III speed to HIGH',13,10,'$'   
   
END start 
