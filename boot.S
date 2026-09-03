.code16
.section .text
.globl _start

_start:
    # Guardar a unidade de boot passada pela BIOS no registador DL
    mov %dl, boot_drive

    # Configurar segmentos base
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0x7c00, %sp

    # Mensagem de arranque
    mov $msg_ok, %si
    call print_string

    # Carregar o Kernel para 0x1000:0x0000 via BIOS (INT 0x13)
    mov $0x1000, %ax
    mov %ax, %es       # ES = 0x1000
    xor %bx, %bx       # BX = 0x0000 (Buffer ES:BX = 0x1000:0x0000)

    mov $0x02, %ah     # Função BIOS: Ler setores
    mov $32, %al       # Número de setores (8KB)
    mov $0, %ch        # Cilindro 0
    mov $2, %cl        # Setor 2 (o setor 1 é o bootloader)
    mov $0, %dh        # Cabeça 0
    mov boot_drive, %dl
    int $0x13          # Chamar BIOS

    jc disk_error      # Se falhar (Carry Flag set), salta

    # Configurar segmentos para o Kernel e saltar
    mov $0x1000, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0xfff0, %sp

    ljmp $0x1000, $0x0000  # Salto para o kernel (_start)

disk_error:
    mov $msg_err, %si
    call print_string
1:  hlt
    jmp 1b

print_string:
    lodsb
    or %al, %al
    jz 2f
    mov $0x0e, %ah
    int $0x10
    jmp print_string
2:
    ret

boot_drive: .byte 0
msg_ok:     .asciz "Bootloader OK! A carregar kernel...\r\n"
msg_err:    .asciz "Erro ao ler do disco!\r\n"

.org 510
.word 0xaa55