from PyQt6 import QtWidgets, QtCore
from BetterGuiSpark import GuiSpark
from PyQt6.QtGui import QFont
from PyQt6.QtCore import QTimer
import mmap

# ============================================================
# MEMORIA COMPARTILHADA - WINDOWS
# ============================================================

NOME = "SMP2"
TAMANHO = 12

try:
    memoria = mmap.mmap(
        -1,
        TAMANHO,
        tagname=NOME,
        access=mmap.ACCESS_WRITE
    )
except Exception as erro:
    raise RuntimeError(
        "Nao foi possivel abrir/criar a memoria compartilhada do Windows."
    ) from erro


def escrever_memoria(texto):
    dados = str(texto).encode("utf-8")

    if len(dados) >= TAMANHO:
        print("Texto muito grande para a memoria compartilhada.")
        return False

    memoria.seek(0)
    memoria.write(dados + b"\0" * (TAMANHO - len(dados)))
    memoria.flush()
    return True


def ler_memoria():
    memoria.seek(0)
    dados = memoria.read(TAMANHO).split(b"\0", 1)[0]
    return dados.decode("utf-8", errors="replace")


# ============================================================
# INTERFACE
# ============================================================

gui = GuiSpark(
    title="Psique Player 2",
    width=900,
    height=650
)

# ============================================================
# TELA DE ESCOLHA DA PALAVRA
# ============================================================

def escolher_palavra():

    dialog = QtWidgets.QDialog(gui.window)

    dialog.setWindowTitle("Configurar Psique")
    dialog.setFixedSize(450, 300)
    dialog.setStyleSheet("""
        QDialog {
            background-color: #12121F;
        }

        QLabel {
            color: #00E5FF;
        }

        QLineEdit {
            background-color: #24243A;
            color: white;
            border: 2px solid #00E5FF;
            border-radius: 8px;
            padding: 10px;
            font-size: 24px;
            font-weight: bold;
        }

        QPushButton {
            background-color: #00E5FF;
            color: #12121F;
            border: none;
            border-radius: 8px;
            padding: 12px;
            font-size: 16px;
            font-weight: bold;
        }

        QPushButton:hover {
            background-color: #00B8D4;
        }
    """)

    layout = QtWidgets.QVBoxLayout(dialog)

    titulo = QtWidgets.QLabel("PSIQUE")
    titulo.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)
    titulo.setFont(QFont("Segoe Script", 32, QFont.Weight.Bold))

    descricao = QtWidgets.QLabel("Escolha a palavra secreta")
    descricao.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)

    entrada = QtWidgets.QLineEdit()
    entrada.setMaxLength(5)
    entrada.setPlaceholderText("Digite 5 letras...")
    entrada.setAlignment(QtCore.Qt.AlignmentFlag.AlignCenter)

    botao = QtWidgets.QPushButton("INICIAR JOGO")

    layout.addWidget(titulo)
    layout.addWidget(descricao)
    layout.addWidget(entrada)
    layout.addWidget(botao)

    resultado = {"palavra": None}

    def confirmar():

        texto = entrada.text().strip().upper()

        if len(texto) != 5 or not texto.isalpha():
            descricao.setText("Digite exatamente 5 letras!")
            return

        resultado["palavra"] = texto
        dialog.accept()

    botao.clicked.connect(confirmar)
    entrada.returnPressed.connect(confirmar)

    if dialog.exec() != QtWidgets.QDialog.DialogCode.Accepted:
        raise SystemExit

    return resultado["palavra"]


# Escolhe a palavra antes de abrir o jogo
palavrateste = escolher_palavra()
palavra = list(palavrateste.upper())

print("PALAVRA DEFINIDA:", palavrateste)

# Configuracoes
color = ["cyan", "cyan", "cyan", "cyan", "cyan"]
letra = ["", "", "", "", ""]
correto = [0, 0, 0, 0, 0]

InputContent = ""
FontColor = "#141414"

gui.set_theme(
    "custom",
    bg="#12121F",
    fg="#F5F5FF",
    widget_bg="#24243A",
    widget_fg="#FFFFFF",
    accent="#00E5FF"
)


# ============================================================
# ENVIO DA TENTATIVA
# ============================================================

def SendToJudge(*args):
    global InputContent

    campo = gui.widgets[Input]["widget"]
    InputContent = campo.text()

    escrever_memoria(InputContent)

    print("[PYTHON -> SMP2]", InputContent)

    avaliar_palavra(InputContent)


# ============================================================
# TITULO
# ============================================================

texto = gui.add_widget(
    "label",
    x=345,
    y=64,
    width=280,
    height=70,
    text="PSIQUE",
    bg="transparent",
    fg="cyan",
    command=None,
    items=None
)

gui.widgets[texto]["widget"].setFont(
    QFont("Segoe Script", 50, QFont.Weight.Bold)
)


# ============================================================
# QUADRADOS
# ============================================================

FundoLetra1 = gui.add_widget(
    "frame", x=82, y=247, width=100, height=100,
    text="", bg=color[0], fg="#ffffff",
    command=None, items=None
)

letra1 = gui.add_widget(
    "label", x=115, y=265, width=50, height=60,
    text=letra[0], bg=color[0], fg=FontColor,
    command=None, items=None
)


FundoLetra2 = gui.add_widget(
    "frame", x=222, y=247, width=100, height=100,
    text="", bg=color[1], fg="#ffffff",
    command=None, items=None
)

letra2 = gui.add_widget(
    "label", x=250, y=265, width=50, height=60,
    text=letra[1], bg=color[1], fg=FontColor,
    command=None, items=None
)


FundoLetra3 = gui.add_widget(
    "frame", x=361, y=247, width=100, height=100,
    text="", bg=color[2], fg="#ffffff",
    command=None, items=None
)

letra3 = gui.add_widget(
    "label", x=385, y=265, width=50, height=60,
    text=letra[2], bg=color[2], fg=FontColor,
    command=None, items=None
)


FundoLetra4 = gui.add_widget(
    "frame", x=519, y=247, width=100, height=100,
    text="", bg=color[3], fg="#ffffff",
    command=None, items=None
)

letra4 = gui.add_widget(
    "label", x=545, y=265, width=50, height=60,
    text=letra[3], bg=color[3], fg=FontColor,
    command=None, items=None
)


FundoLetra5 = gui.add_widget(
    "frame", x=675, y=247, width=100, height=100,
    text="", bg=color[4], fg="#ffffff",
    command=None, items=None
)

letra5 = gui.add_widget(
    "label", x=700, y=265, width=50, height=60,
    text=letra[4], bg=color[4], fg=FontColor,
    command=None, items=None
)


# ============================================================
# REFERENCIAS DOS QUADRADOS
# ============================================================

QUADRADOS = [
    letra1,
    letra2,
    letra3,
    letra4,
    letra5
]

FUNDOS = [
    FundoLetra1,
    FundoLetra2,
    FundoLetra3,
    FundoLetra4,
    FundoLetra5
]


# ============================================================
# AVALIACAO DA PALAVRA
# ============================================================

def avaliar_palavra(texto):

    global letra, correto, color

    tentativa = list(str(texto).strip().upper())

    # Limita a tentativa a cinco caracteres
    tentativa = tentativa[:5]

    for i in range(5):

        if i < len(tentativa):

            letra[i] = tentativa[i]

            # Compara a letra com a posicao correspondente
            if tentativa[i] == palavra[i]:
                correto[i] = 1
                color[i] = "green"

            else:
                correto[i] = 0
                color[i] = "red"

        else:

            letra[i] = ""
            correto[i] = 0
            color[i] = "cyan"

        # Atualiza o texto do label
        widget = gui.widgets[QUADRADOS[i]]["widget"]
        widget.setText(letra[i])

        # Atualiza a cor do fundo e do label
        fundo = gui.widgets[FUNDOS[i]]["widget"]

        fundo.setStyleSheet(
            f"background-color: {color[i]};"
        )

        widget.setStyleSheet(
            f"background-color: {color[i]}; color: {FontColor};"
        )

    #print("Tentativa:", tentativa)
    #print("Resultado:", correto)
    #print("Cores:", color)

    if correto == [1, 1, 1, 1, 1]:
        print("PALAVRA CORRETA!")
    else:
        print("PALAVRA INCORRETA!")

# ============================================================
# RODAPE
# ============================================================

footer = gui.add_widget(
    "frame",
    x=0,
    y=570,
    width=900,
    height=100,
    text="",
    bg="#ffffff",
    fg="#ffffff",
    command=None,
    items=None
)


DebugText = gui.add_widget(
    "label",
    x=25,
    y=580,
    width=850,
    height=25,
    text="Memoria: vazia",
    bg="#ffffff",
    fg="#000000",
    command=None,
    items=None
)


# ============================================================
# CAMPO DE ENTRADA
# ============================================================

Input = gui.add_widget(
    "entry",
    x=250,
    y=500,
    width=400,
    height=50,
    text=InputContent,
    bg="white",
    fg="#000000",
    command=None,
    items=None
)


# ============================================================
# BOTAO
# ============================================================

Sendbt = gui.add_widget(
    "button",
    x=650,
    y=500,
    width=100,
    height=50,
    text="➤",
    bg="#00E5FF",
    fg="#000000",
    command=SendToJudge,
    items=None
)

gui.add_shortcut("Return", SendToJudge)
gui.add_shortcut("Enter", SendToJudge)


# ============================================================
# FONTES
# ============================================================

gui.set_font_size(Sendbt, 20)

gui.set_font_size(letra1, 50)
gui.set_font_size(letra2, 50)
gui.set_font_size(letra3, 50)
gui.set_font_size(letra4, 50)
gui.set_font_size(letra5, 50)

gui.set_font_size(footer, 10)


# ============================================================
# MONITORAMENTO DA MEMORIA
# ============================================================

debug_widget = gui.widgets[DebugText]["widget"]
ultimo_texto = [""]


def atualizar_debug():

    try:

        conteudo = ler_memoria()

        if conteudo != ultimo_texto[0]:

            ultimo_texto[0] = conteudo

            debug_widget.setText("Memoria: " + conteudo)

            # Avalia tambem alteracoes recebidas pela memoria
            avaliar_palavra(conteudo)

    except (ValueError, BufferError):
        pass


timer_memoria = QTimer()
timer_memoria.timeout.connect(atualizar_debug)
timer_memoria.start(100)


# ============================================================
# EXECUCAO
# ============================================================

try:

    gui.run()

finally:

    timer_memoria.stop()
    memoria.close()
