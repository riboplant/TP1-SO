make clean
# Generar archivo de registro durante la compilación
pvs-studio-analyzer trace -- make all

# Analizar el proyecto y generar reporte
pvs-studio-analyzer analyze -o report.plog

# Convertir el reporte a formato HTML
plog-converter -a GA:1,2 -t html report.plog -o report.html

# Abrir el reporte en un navegador web
xdg-open report.html
