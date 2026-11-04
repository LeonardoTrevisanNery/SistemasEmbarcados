import tkinter as tk

# Paleta inspirada no Termo (tema escuro)
BG, TILE, TILE_ACTIVE = "#1c1a1b", "#312a2c", "#615458"
CERTO, LUGAR, ERRADO = "#3aa394", "#d3ad69", "#4a4144"
TXT, KEY = "#ffffff", "#816f75"
LINHAS, COLUNAS = 6, 5
TECLAS = ["QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"]

linha, atual = 0, []  # linha ativa e letras digitadas nela

def send_to_judge(palavra):
    # enfia o texto pro juiz.c
    status.config(text=f"Enviado: {palavra}")

def pintar(row, cores):
    """Colore uma linha. Use CERTO / LUGAR / ERRADO conforme a resposta do juiz."""
    for tile, cor in zip(tiles[row], cores):
        tile.config(bg=cor, highlightbackground=cor)

def atualizar():
    for c, tile in enumerate(tiles[linha]):
        tile.config(text=atual[c] if c < len(atual) else "",
                    highlightbackground=TILE_ACTIVE if c == len(atual) else TILE)

def tecla(t):
    global linha, atual
    if linha >= LINHAS:
        return
    if t == "⌫":
        atual = atual[:-1]
    elif t == "ENTER":
        if len(atual) < COLUNAS:
            return status.config(text="Faltam letras")
        send_to_judge("".join(atual))
        pintar(linha, [ERRADO] * COLUNAS)  # placeholder até o juiz responder
        linha, atual = linha + 1, []
        if linha < LINHAS:
            atualizar()
        return
    elif len(atual) < COLUNAS:
        atual.append(t)
    atualizar()

root = tk.Tk()
root.title("Psique")
root.geometry("600x780")
root.configure(bg=BG)

tk.Label(root, text="psique", bg=BG, fg=TXT,
         font=("Helvetica", 38, "bold")).pack(pady=(25, 15))

# Grade de tentativas
grade = tk.Frame(root, bg=BG)
grade.pack()
tiles = [[tk.Label(grade, bg=TILE, fg=TXT, width=2, font=("Helvetica", 30, "bold"),
                   highlightthickness=3, highlightbackground=TILE)
          for _ in range(COLUNAS)] for _ in range(LINHAS)]
for r, row in enumerate(tiles):
    for c, t in enumerate(row):
        t.grid(row=r, column=c, padx=4, pady=4, ipady=8)

status = tk.Label(root, text="", bg=BG, fg=LUGAR, font=("Helvetica", 12))
status.pack(pady=12)

# Teclado na tela
teclado = tk.Frame(root, bg=BG)
teclado.pack(side="bottom", pady=25)
for i, letras in enumerate(TECLAS):
    f = tk.Frame(teclado, bg=BG)
    f.pack(pady=3)
    chaves = (["ENTER"] if i == 2 else []) + list(letras) + (["⌫"] if i == 2 else [])
    for k in chaves:
        tk.Button(f, text=k, bg=KEY, fg=TXT, activebackground=TILE_ACTIVE,
                  activeforeground=TXT, relief="flat", bd=0,
                  width=6 if len(k) > 1 else 3, font=("Helvetica", 13, "bold"),
                  command=lambda k=k: tecla(k)).pack(side="left", padx=2, ipady=8)

# Teclado físico
root.bind("<Return>", lambda _: tecla("ENTER"))
root.bind("<BackSpace>", lambda _: tecla("⌫"))
root.bind("<Key>", lambda e: e.char.isalpha() and e.char.isascii() and tecla(e.char.upper()))

atualizar()
root.mainloop()
