from BetterGuiSpark import GuiSpark
from PyQt6.QtGui import QFont
gui = GuiSpark(
    title="Nova Interface",
    width=900,
    height=650
)


teste=""
#Configurações de Aplicação
color=["green","orange","red","green","orange"]
letra = ["L","E","G","A","L"]
FontColor = "#141414"
gui.set_theme("custom",
              bg="#12121F",
              fg="#F5F5FF", 
              widget_bg="#24243A",
              widget_fg="#FFFFFF",
              accent="#00E5FF"
              )

def SendToJudge():
   #enfia o texto pro juiz.c
   pass
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
gui.widgets[texto]["widget"].setFont(QFont("Segoe Script", 50,QFont.Weight.Bold))

FundoLetra1 = gui.add_widget(
    "frame",
    x=82,
    y=247,
    width=100,
    height=100,
    text="",
    bg=color[0],
    fg="#ffffff",
    command=None,
    items=None
)
letra1 = gui.add_widget(
    "label",
    x=115,
    y=265,
    width=50,
    height=60,
    text=letra[0],
    bg=color[0],
    fg=FontColor,
    command=None,
    items=None
)

FundoLetra2 = gui.add_widget(
    "frame",
    x=222,
    y=247,
    width=100,
    height=100,
    text="",
    bg=color[1],
    fg="#ffffff",
    command=None,
    items=None
)
letra2 = gui.add_widget(
    "label",
    x=250,
    y=265,
    width=50,
    height=60,
    text=letra[1],
    bg=color[1],
    fg=FontColor,
    command=None,
    items=None
)

FundoLetra3 = gui.add_widget(
    "frame",
    x=361,
    y=247,
    width=100,
    height=100,
    text="",
    bg=color[2],
    fg="#ffffff",
    command=None,
    items=None
)
letra3 = gui.add_widget(    
    
    "label",
    x=385,
    y=265,
    width=50,
    height=60,
    text=letra[2],
    bg=color[2],
    fg=FontColor,
    command=None,
    items=None
)
FundoLetra4 = gui.add_widget(
    "frame",
    x=519,
    y=247,
    width=100,
    height=100,
    text="",
    bg=color[3],
    fg="#ffffff",
    command=None,
    items=None
)
letra4 = gui.add_widget(
    "label",
    x=545,
    y=265,
    width=50,
    height=60,
    text=letra[3],
    bg=color[3],
    fg=FontColor,
    command=None,
    items=None
)

FundoLetra5 = gui.add_widget(
    "frame",
    x=675,
    y=247,
    width=100,
    height=100,
    text="",
    bg=color[4],
    fg="#ffffff",
    command=None,
    items=None
)
letra5 = gui.add_widget(
    "label",
    x=700,
    y=265,
    width=50,
    height=60,
    text=letra[4],
    bg=color[4],
    fg=FontColor,
    command=None,
    items=None
)
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
    x=815,
    y=580,
    width=900,
    height=15,
    text=teste,
    bg="#ffffff",
    fg="#000000",
    command=None,
    items=None
 )
Input= gui.add_widget(
    "entry",
    x=250,
    y=500,
    width=400,
    height=50,
    text="",
    bg="white",
    fg="#000000",
    command=None,
    items=None
 )
Sendbt=gui.add_widget(
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
gui.set_font_size(Sendbt, 20)
#Setando o tamanho das letras dentro dos quadrados coloridos
gui.set_font_size(letra1, 50)
gui.set_font_size(letra2, 50)
gui.set_font_size(letra3, 50)
gui.set_font_size(letra4, 50)
gui.set_font_size(letra5, 50)
gui.set_font_size(footer, 10)
gui.run()
