import os
import sys
from reportlab.lib.pagesizes import A4
from reportlab.lib import colors
from reportlab.lib.units import mm
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, KeepTogether, HRFlowable
)
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.enums import TA_CENTER, TA_LEFT, TA_RIGHT
from reportlab.pdfgen import canvas

class NumberedCanvas(canvas.Canvas):
    def __init__(self, *args, **kwargs):
        super(NumberedCanvas, self).__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_decorations(num_pages)
            super(NumberedCanvas, self).showPage()
        super(NumberedCanvas, self).save()

    def draw_page_decorations(self, page_count):
        self.saveState()
        self.setFont("Helvetica", 8)
        self.setFillColor(colors.HexColor("#718096"))
        
        # Header (pages > 1)
        if self._pageNumber > 1:
            self.drawString(15 * mm, 285 * mm, "Schnapshäcksler 1:32 — Hardware- & Platinen-Masterplan")
            self.setStrokeColor(colors.HexColor("#CBD5E0"))
            self.setLineWidth(0.5)
            self.line(15 * mm, 282 * mm, 195 * mm, 282 * mm)
            
        # Footer
        page_str = f"Seite {self._pageNumber} von {page_count}"
        self.drawRightString(195 * mm, 12 * mm, page_str)
        self.drawString(15 * mm, 12 * mm, "Vertraulich / Projekt-Dokumentation — Stand: September 2026")
        self.setStrokeColor(colors.HexColor("#CBD5E0"))
        self.setLineWidth(0.5)
        self.line(15 * mm, 16 * mm, 195 * mm, 16 * mm)
        self.restoreState()

def build_pdf(filename):
    doc = SimpleDocTemplate(
        filename,
        pagesize=A4,
        leftMargin=15 * mm,
        rightMargin=15 * mm,
        topMargin=20 * mm,
        bottomMargin=20 * mm
    )

    styles = getSampleStyleSheet()
    
    # Custom styles
    primary_color = colors.HexColor("#2B6CB0") # Professional Slate Blue
    accent_green = colors.HexColor("#2F855A")  # Agro Green
    dark_slate = colors.HexColor("#1A202C")
    light_bg = colors.HexColor("#F7FAFC")
    border_color = colors.HexColor("#E2E8F0")

    title_style = ParagraphStyle(
        'DocTitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=20,
        leading=24,
        textColor=dark_slate,
        alignment=TA_LEFT
    )

    subtitle_style = ParagraphStyle(
        'DocSubtitle',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=11,
        leading=15,
        textColor=colors.HexColor("#4A5568"),
        alignment=TA_LEFT
    )

    h1_style = ParagraphStyle(
        'Heading1_Custom',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=13,
        leading=17,
        textColor=primary_color,
        spaceBefore=12,
        spaceAfter=6
    )

    h2_style = ParagraphStyle(
        'Heading2_Custom',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=10.5,
        leading=14,
        textColor=dark_slate,
        spaceBefore=8,
        spaceAfter=4
    )

    body_style = ParagraphStyle(
        'Body_Custom',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=9,
        leading=13,
        textColor=dark_slate
    )

    badge_style = ParagraphStyle(
        'Badge',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=8,
        leading=10,
        textColor=colors.white,
        alignment=TA_CENTER
    )

    table_header_style = ParagraphStyle(
        'TableHeader',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=8.5,
        leading=11,
        textColor=colors.white
    )

    table_cell_style = ParagraphStyle(
        'TableCell',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8.5,
        leading=11,
        textColor=dark_slate
    )

    table_cell_bold = ParagraphStyle(
        'TableCellBold',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=8.5,
        leading=11,
        textColor=dark_slate
    )

    table_cell_code = ParagraphStyle(
        'TableCellCode',
        parent=styles['Normal'],
        fontName='Courier-Bold',
        fontSize=8.5,
        leading=11,
        textColor=colors.HexColor("#C53030")
    )

    story = []

    # Title Banner
    story.append(Paragraph("🚜 AUTOMATED FLUID DISPENSER (SCHNAPSHÄCKSLER 1:32)", title_style))
    story.append(Spacer(1, 2 * mm))
    story.append(Paragraph("<b>Hardware-Masterplan & Platinen-Belegungsplan (V2.0 — ESP32 Architektur)</b>", subtitle_style))
    story.append(Spacer(1, 1 * mm))
    story.append(HRFlowable(width="100%", thickness=2, color=accent_green, spaceAfter=10, spaceBefore=4))

    # Intro box
    intro_text = """
    Dieser Plan definiert die Spezifikation für das neue zentrale <b>Mainboard (2-in-1 PCB)</b>. 
    Das System trennt strikt zwischen <b>Laststrom / Spannungsversorgung (robuste Schraubklemmen)</b> 
    und <b>Steuer-/Messsignalen (verriegelte JST-XH 2,54 mm Buchsen)</b>. Es integriert Dual-Power-Versorgung 
    (USB-C PD + DC-Buchse), Unterspannungs-Wächterschaltung, 5A Step-Down Wandler, ESP32-Microcontroller und MOSFET-Pumpentreiber.
    """
    
    intro_table = Table([[Paragraph(intro_text, body_style)]], colWidths=[180 * mm])
    intro_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor("#EDF2F7")),
        ('BOX', (0,0), (-1,-1), 1, colors.HexColor("#CBD5E0")),
        ('PADDING', (0,0), (-1,-1), 8),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
    ]))
    story.append(intro_table)
    story.append(Spacer(1, 4 * mm))

    # Section 1: BLOCK A - Schraubklemmen
    story.append(Paragraph("1. BLOCK A: Schraubklemmen (Stromversorgung & Lastkreise)", h1_style))
    story.append(Paragraph("Für alle dicken Stromversorgungs- und Lastleitungen (empfohlener Querschnitt: 0,5 bis 0,75 mm²).", body_style))
    story.append(Spacer(1, 2 * mm))

    block_a_data = [
        [
            Paragraph("Klemme", table_header_style),
            Paragraph("Bezeichnung", table_header_style),
            Paragraph("Spannung / Strom", table_header_style),
            Paragraph("Kabel-Ziel / Funktion", table_header_style)
        ],
        [
            Paragraph("<b>TB1</b> (2-Pin)", table_cell_bold),
            Paragraph("12V USB-C PD IN", table_cell_code),
            Paragraph("12V DC (max. 3A)", table_cell_style),
            Paragraph("Eingang vom USB-C Power Delivery Trigger Board", table_cell_style)
        ],
        [
            Paragraph("<b>TB2</b> (2-Pin)", table_cell_bold),
            Paragraph("12V DC-BUCHSE IN", table_cell_code),
            Paragraph("12V DC (max. 3A)", table_cell_style),
            Paragraph("Eingang von der 5,5 × 2,1 mm Rundbuchse (Tischnetzteil)", table_cell_style)
        ],
        [
            Paragraph("<b>TB3</b> (2-Pin)", table_cell_bold),
            Paragraph("12V PUMPEN-MOTOR", table_cell_code),
            Paragraph("12V geschaltet (MOSFET)", table_cell_style),
            Paragraph("Direktanschluss an die 12V-Membran-/Schlauchpumpe (mit Freilaufdiode)", table_cell_style)
        ],
        [
            Paragraph("<b>TB4</b> (2-Pin)", table_cell_bold),
            Paragraph("5V HÄCKSLER POWER", table_cell_code),
            Paragraph("5.0V DC (max. 2.5A)", table_cell_style),
            Paragraph("Dicke 5V/GND Speisung ins Modell (für Servo & Front/Back-LEDs)", table_cell_style)
        ],
        [
            Paragraph("<b>TB5</b> (2-Pin)", table_cell_bold),
            Paragraph("5V WS2811 POWER", table_cell_code),
            Paragraph("5.0V DC (max. 1.5A)", table_cell_style),
            Paragraph("Dicke 5V/GND Speisung zu den Lichtkammern der 5 Stellplätze", table_cell_style)
        ],
        [
            Paragraph("<b>TB6</b> (1-Pin)", table_cell_bold),
            Paragraph("TASTER MASSE (GND)", table_cell_code),
            Paragraph("System-GND (0V)", table_cell_style),
            Paragraph("Zentraler Masseanschluss für die 3 durchgeschliffenen Taster", table_cell_style)
        ]
    ]

    t_block_a = Table(block_a_data, colWidths=[28 * mm, 42 * mm, 38 * mm, 72 * mm])
    t_block_a.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), primary_color),
        ('ALIGN', (0,0), (-1,-1), 'LEFT'),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('GRID', (0,0), (-1,-1), 0.5, border_color),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, light_bg]),
        ('TOPPADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
    ]))
    story.append(t_block_a)
    story.append(Spacer(1, 4 * mm))

    # Section 2: BLOCK B - JST-XH Stecker
    story.append(Paragraph("2. BLOCK B: JST-XH 2,54 mm Stecker (Steuer- & Sensorsignale)", h1_style))
    story.append(Paragraph("Verriegelte Klick-Stecker für störsichere und vibrationsfeste Signalübertragung.", body_style))
    story.append(Spacer(1, 2 * mm))

    block_b_data = [
        [
            Paragraph("Stecker", table_header_style),
            Paragraph("Pins / Signalbelegung", table_header_style),
            Paragraph("ESP32 Pin", table_header_style),
            Paragraph("Funktion & Details", table_header_style)
        ],
        [
            Paragraph("<b>J1</b><br/>(4-Pin JST)", table_cell_bold),
            Paragraph("Pin 1: Servo PWM<br/>Pin 2: RKL PWM<br/>Pin 3: Turm-Lampen PWM<br/>Pin 4: Front/Back PWM", table_cell_style),
            Paragraph("GPIO 19<br/>GPIO 25<br/>GPIO 17<br/>GPIO 26", table_cell_code),
            Paragraph("<b>Häcksler-Signalkabelbaum:</b><br/>Steuert Servomotor (Turmdrehung), Rundumleuchte, Turmstrahler und Arbeitsscheinwerfer.", table_cell_style)
        ],
        [
            Paragraph("<b>J2</b><br/>(2-Pin JST)", table_cell_bold),
            Paragraph("Pin 1: WS2811 DATA<br/>Pin 2: GND (Schirmung)", table_cell_style),
            Paragraph("GPIO 18<br/>GND", table_cell_code),
            Paragraph("<b>Lichtkammer-Daten (Option A):</b><br/>800 kHz Datensignal zu Platz 1 (mit 330 Ω Serien-Schutzwiderstand auf PCB).", table_cell_style)
        ],
        [
            Paragraph("<b>J3</b><br/>(4-Pin JST)", table_cell_bold),
            Paragraph("Pin 1: 3.3V VCC<br/>Pin 2: GND<br/>Pin 3: SDA (I2C Daten)<br/>Pin 4: SCL (I2C Takt)", table_cell_style),
            Paragraph("3V3<br/>GND<br/>GPIO 21<br/>GPIO 22", table_cell_code),
            Paragraph("<b>Sensorboard Verbindung:</b><br/>I2C-Busleitung nach vorne zum Adafruit MPR121 Modul unter den 5 Gläser-Stellplätzen.", table_cell_style)
        ],
        [
            Paragraph("<b>J4</b><br/>(2-Pin JST)", table_cell_bold),
            Paragraph("Pin 1: Taster Schließer<br/>Pin 2: Taster-Lampe (+)", table_cell_style),
            Paragraph("GPIO 23 (In)<br/>GPIO 5 (PWM)", table_cell_code),
            Paragraph("<b>Start-Taster:</b> Startet den automatischen Abfüllvorgang. LED pulsiert bei Betriebsbereitschaft.", table_cell_style)
        ],
        [
            Paragraph("<b>J5</b><br/>(2-Pin JST)", table_cell_bold),
            Paragraph("Pin 1: Taster Schließer<br/>Pin 2: Taster-Lampe (+)", table_cell_style),
            Paragraph("GPIO 32 (In)<br/>GPIO 16 (PWM)", table_cell_code),
            Paragraph("<b>Pump-Taster:</b> Manuelles Spülen / Vorpumpen bei Gedrückthalten.", table_cell_style)
        ],
        [
            Paragraph("<b>J6</b><br/>(2-Pin JST)", table_cell_bold),
            Paragraph("Pin 1: Taster Schließer<br/>Pin 2: Taster-Lampe (+)", table_cell_style),
            Paragraph("GPIO 33 (In)<br/>GPIO 4 (PWM)", table_cell_code),
            Paragraph("<b>Lampen-Taster:</b> Schaltet die Arbeits- und Show-Beleuchtung ein/um.", table_cell_style)
        ]
    ]

    t_block_b = Table(block_b_data, colWidths=[24 * mm, 48 * mm, 32 * mm, 76 * mm])
    t_block_b.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), primary_color),
        ('ALIGN', (0,0), (-1,-1), 'LEFT'),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('GRID', (0,0), (-1,-1), 0.5, border_color),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, light_bg]),
        ('TOPPADDING', (0,0), (-1,-1), 3),
        ('BOTTOMPADDING', (0,0), (-1,-1), 3),
    ]))
    story.append(t_block_b)

    story.append(PageBreak())

    # Section 3: BLOCK C - On-Board Komponenten & Schaltungen
    story.append(Paragraph("3. BLOCK C: On-Board Komponenten & Schaltungsdetails", h1_style))
    story.append(Spacer(1, 1 * mm))

    comp_data = [
        [
            Paragraph("Baugruppe", table_header_style),
            Paragraph("Bauteil / Spezifikation", table_header_style),
            Paragraph("Einbau & Wichtige Hinweise", table_header_style)
        ],
        [
            Paragraph("<b>Mikrocontroller</b>", table_cell_bold),
            Paragraph("ESP32 DevKit V1 (30-Pin)<br/>ESP-WROOM-32 Dual Core", table_cell_style),
            Paragraph("Wird auf 2x 15-Pin Buchsenleisten gesteckt. 5V-Versorgung erfolgt über den <code>VIN</code>-Pin. 3,3V für MPR121 kommt vom <code>3V3</code>-Pin.", table_cell_style)
        ],
        [
            Paragraph("<b>Step-Down Wandler</b>", table_cell_bold),
            Paragraph("XL4015 5A CC/CV Modul<br/>(Rot/Oranges Board)", table_cell_style),
            Paragraph("Wird mit 4x M3 Distanzbolzen über das Mainboard geschraubt. <b>WICHTIG:</b> CC-Poti (Strombegrenzung) voll im Uhrzeigersinn auf Maximum (5A) drehen! CV-Poti auf exakt <b>5,05 V</b> einstellen.", table_cell_style)
        ],
        [
            Paragraph("<b>Wächter & Schutz</b>", table_cell_bold),
            Paragraph("2x SS34 / SS54 Schottky<br/>+ Unterspannungsüberwachung", table_cell_style),
            Paragraph("Schützt vor Rückspeisung zwischen USB-C PD und DC-Hohlbuchse. Schaltet die 12V-Hauptschiene erst frei, wenn echte 11,5–12,0 V anliegen (Verhindert 5V-Fehlversuch bei USB-C).", table_cell_style)
        ],
        [
            Paragraph("<b>Pumpen-Treiber</b>", table_cell_bold),
            Paragraph("N-Channel Logic MOSFET<br/>(IRLZ44N oder AO3400A)<br/>+ Freilaufdiode 1N4007", table_cell_style),
            Paragraph("Ersetzt das mechanische Relais. Gate an GPIO 27 mit 10 kΩ Pulldown gegen GND. 1N4007 Freilaufdiode antiparallel zum Pumpenanschluss TB3 schützt vor Induktionsspitzen.", table_cell_style)
        ],
        [
            Paragraph("<b>Puffer-Kondensator</b>", table_cell_bold),
            Paragraph("1000 µF / 16V (oder 25V) Elko<br/>Low-ESR", table_cell_style),
            Paragraph("Sitzt direkt parallel zwischen 5V und GND am Ausgang des XL4015. Puffert Stromspitzen beim Servolauf und plötzlichem Weißaufblitzen der LEDs zuverlässig ab.", table_cell_style)
        ]
    ]

    t_comp = Table(comp_data, colWidths=[35 * mm, 50 * mm, 95 * mm])
    t_comp.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), accent_green),
        ('ALIGN', (0,0), (-1,-1), 'LEFT'),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
        ('GRID', (0,0), (-1,-1), 0.5, border_color),
        ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, light_bg]),
        ('TOPPADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
    ]))
    story.append(t_comp)
    story.append(Spacer(1, 4 * mm))

    # Section 4: Lichtkammer & Sensorboard
    story.append(Paragraph("4. Peripherie-Platinen: Lichtkammer-PCBs & Sensorboard", h1_style))
    story.append(Spacer(1, 1 * mm))

    peri_text = """
    <b>A. 5x Lichtkammer-Mini-PCBs (für den Boden der 3D-Druck-Lichtschächte):</b><br/>
    • Kleine runde/quadratische Platine mit 4x WS2811 5mm LEDs in Reihe geschaltet.<br/>
    • <b>Eingang (links):</b> 5V, GND, DIN | <b>Ausgang (rechts):</b> 5V, GND, DOUT (Daisy-Chain zu Platz 2, 3, 4, 5).<br/>
    • <b>Vorteil:</b> Exakt reproduzierbarer Abstrahlwinkel, keine losen Widerstände, 1,5 Stunden Zeitersparnis beim Löten.<br/><br/>
    <b>B. Sensorboard (Adafruit MPR121 Breakout nah an den Gläsern):</b><br/>
    • Schmale Platine direkt unter der Abstellfläche, verbunden über Stecker <b>J3</b> mit dem Mainboard.<br/>
    • 5 kurze Lötpads direkt am Rand zu den 5 Kupferfolien (Leitungslänge &lt; 2 cm für maximale kapazitive Störsicherheit).
    """

    peri_table = Table([[Paragraph(peri_text, body_style)]], colWidths=[180 * mm])
    peri_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor("#F0FFF4")),
        ('BOX', (0,0), (-1,-1), 1, colors.HexColor("#9AE6B4")),
        ('PADDING', (0,0), (-1,-1), 8),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
    ]))
    story.append(peri_table)
    story.append(Spacer(1, 4 * mm))

    # Section 5: Checkliste für EasyEDA / KiCad
    story.append(Paragraph("5. Checkliste für die PCB-Bestellung bei JLCPCB (2-in-1 Panel)", h1_style))
    story.append(Spacer(1, 1 * mm))

    check_text = """
    <b>1. Panel-Größe:</b> Zeichne das Hauptboard (ca. 75 × 100 mm) und das Sensorboard (ca. 25 × 100 mm) auf eine <b>100 × 100 mm Gesamtfläche</b> (kostet bei JLCPCB nur ~2 $ für 5 Stück).<br/>
    <b>2. Trennung:</b> Verbinde beide Bereiche mit einer Lochreihe (Mouse Bites) zum einfachen Abbrechen per Hand.<br/>
    <b>3. Leiterbahnen-Breiten:</b> 12V- und 5V-Stromschienen mit min. <b>1,5 mm bis 2,0 mm</b> Leiterbahnbreite (oder Polygon-Pour) ausführen. Signalbahnen mit <b>0,25 mm bis 0,3 mm</b>.<br/>
    <b>4. Massefläche:</b> Ober- und Unterseite großflächig mit einer durchgehenden <b>GND-Kupferfläche (Ground Plane)</b> füllen.
    """

    check_table = Table([[Paragraph(check_text, body_style)]], colWidths=[180 * mm])
    check_table.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor("#EDF2F7")),
        ('BOX', (0,0), (-1,-1), 1, colors.HexColor("#CBD5E0")),
        ('PADDING', (0,0), (-1,-1), 8),
        ('VALIGN', (0,0), (-1,-1), 'MIDDLE'),
    ]))
    story.append(check_table)

    doc.build(story, canvasmaker=NumberedCanvas)
    print(f"PDF successfully created: {filename}")

if __name__ == '__main__':
    out_path = "/Users/jonaswill/Library/Mobile Documents/com~apple~CloudDocs/Projekte/Automated-Fluid-Dispenser/Automated-Fluid-Dispenser/Dokumente/Masterplan_Platine_Belegung.pdf"
    build_pdf(out_path)
