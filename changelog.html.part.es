	<li>0.22: El teclado se pinta con degradados. Se controla con la opción USE_GRADIENTS, cuyo valor por defecto es 1.</li>
	<li>0.22: Se corrigió el bug #432887: A veces el programa se lanza múltiples veces. (Eduardo Bustamante)</li>
	<li>0.22: Superkb ya no usa múltiples procesos. (Eduardo Bustamante)</li>
	<li>0.22: Se corrigió el enlazado con versiones nuevas de GCC por falta de inclusión explícita de libdl. (Eduardo Bustamante)</li>
	<li>0.22: Superkb tratará de detectar detalles sobre el estado del código fuente y proveer más información en la versión. Ejemplo: 0.21+git-0123456+localchanges(b:master).</li>
	<li>0.22: Se corrigió el cálculo de posicionamiento de los iconos.</li>
	<li>0.22: Varios crashes corregidos.</li>
	<li>0.21: Detección de teclas Super que no se puedan usar e información al respecto en la terminal.</li>
	<li>0.21: YA no se depende de que SIGCHLD se ignore. (Corrige los problemas con chromium-browser y Gwibber).</li>
	<li>0.21: Un mensaje amigable se muestra en pantalla cuando Superkb carga correctamente (mediante la nueva opcion WELCOME_CMD).</li>
	<li>0.21: Las cadenas de retroalimentación ya no necesitan ser entrecomilladas dos veces. (mediante la nueva opción FEEDBACK_STRINGS_AUTOQUOTE).</li>
	<li>0.21: Los "estados" (como Shift) ya se pueden usar.</li>
	<li>0.21: Mucho trabajo en portabilidad de código.</li>
	<li>0.21: Muchos comentarios a lo largo del código.</li>
	<li>0.21: Muchas correcciones de usabilidad y estabilidad.</li>
	<li>0.20: Se cambió el estilo por omisión para pintar las figuras a FLAT_KEY.</li>
	<li>0.20: Se cambió el método por omisión para pintar a Cairo y el tipo a "Sans Bold".</li>
	<li>0.20: Soporte para pintar con Cairo.</li>
	<li>0.20: Corregidos los crashes con teclas sin etiqueta.</li>
	<li>0.20: Corregido el BadAccess en XChangeKeyboardControl cuando SUPERKEY_CODE es 0.</li>
	<li>0.20: Modularización de las bibliotecas drawkb.</li>
	<li>0.20: Mensajes de errores más amigables para guiar durante la instalación.</li>
	<li>0.20: Se corrigieron fallas al detectar la geometría del teclado.</li>
	<li>0.20: Se añadió una página de manual simple.</li>
	<li>0.20: Se permite una configuración a nivel sistema en /etc/superkbrc.</li>
	<li>0.20: Se corrigió la compilación para x86_64 (faltaba -fPIC).</li>
	<li>0.17: Cambiamos de SCM a Git.</li>
	<li>0.17: Marcar las ventanas como TRANSIENT cada que se aparezcan.</li>
	<li>0.17: Soporte para EINTR en el select() de superkb.c.</li>
	<li>0.17: Soporte para Xinerama.</li>
	<li>0.17: Errores amigables al compilar, instalar y cargar configuraci�n.</li>
	<li>0.16: Algunos bichos corregidos, reportados por gcc -Wextra.</li>
	<li>0.16: Reescrito el c�digo de dibujado de textos mediante Xft.</li>
	<li>0.16: Corregida una condici�n de carrera en el procesamiento de los eventos de X.</li>
	<li>0.15: Se añadió la opción -0 al comando Superkb para salirse cuando esté listo, para efectos de medición.</li>
	<li>0.15: Se reescribió el código para dibujar las etiquetas. Ahora tienen posición y tamaños correctos para cada uno de los métodos de dibujado.</li>
	<li>0.15: Más reescritura del código de búsqueda de etiquetas. Se añadieron nilde, plus y otras.</li>
	<li>0.15: Se reescribió el código para obtener las etiquetas a partir de los KeyString. Sin mejoras.</li>
	<li>0.15: Cambio experimental para corregir un código absurdo. Podría traer bugs.</li>
	<li>0.15: Se añadieron mensajes de depuración a superkb.c</li>
	<li>0.15: Se añadió el soporte para mensajes de depuración mediante la opción "-d nivel".</li>
	<li>0.15: Cambiaron los valores por defecto de color de primer plano y fondo.</li>
	<li>0.15: Cambió el valor por defecto de SUPERKEY_RELEASE_CANCELS a 0.</li>
	<li>0.15: Cambió el valor por defecto de SUPERKEY_REPLAY a 1.</li>
	<li>0.15: Se añadieron dos métodos de dibujado, seleccionables con la directiva DRAWKB_PAINTING_MODE (FULL_SHAPE por defecto), pero puede tomar los valores BASE_OUTLINE_ONLY y FLAT_KEY.</li>
	<li>0.14: $(DESTDIR) no estaba aplicado en el mkdir en el archivo Makefile, por lo que el paquete de Ubuntu se instalaba mal.</li>
	<li>0.13: Empujar versión a 0.13</li>
	<li>0.13: Permitir que Makefile maneje la variable $(DESTDIR).</li>
	<li>0.13: Soporte para SUPERKEY_RELEASE_CANCELS, que permite que Superkb ejecute las acciones cuando las teclas sigan presionadas al soltar la tecla Super.</li>
	<li>0.13: Si los tres componentes de color en BACKGROUND o FOREGORUND son <= 255, las considera del rango 0..255.</li>
	<li>0.13: Empujar versión a 0.12+svn</li>
	<li>0.12: Se antepuso ./ al revisar los resultados de la creación de configuración (evitar el doblem 'make').</li>
	<li>0.12: Suprimir la salida de los comandos al crear la configuración.</li>
	<li>0.12: Nueva opción "SUPERKEY_REPLAY": Configura si la tecla Super debe ser reenviada a la ventana que tenía el enfoque al ser presionada (es decir, no combinada ni mantenida.)</li>
	<li>0.12: Soporte básico para FEEDBACK_HANDLER, para permitir que el usuario sea notificado de su decisión al correr un programa.</li>
	<li>0.11: No más sobrantes zombie ("defunct").</li>
	<li>0.11: Bicho corregido donde una tecla desconocida hacía que la configuración se esparciera por todo el teclado.</li>
	<li>0.11: Soporte para KEY DOCUMENT y DOCUMENT_HANDLER.</li>
	<li>0.10: Los mensajes de error más comunes son más amigables.</li>
	<li>0.10: Se le puede configurar el color de fondo y el color de frente. Ya se ve correctamente. (Los colores son las tres componentes: rojo, verde y azul, del 0 al 65535.)</li>
	<li>0.10: Se les pueden configurar las teclas Super. Esto es para los usuarios de las Thinkpad (como yo).</li>
	<li>0.10: Ahora se puede usar Imlib2 como librería para cargar iconos: puede usarse Gdk-Pixbuf o Imlib2.</li>
	<li>0.10: Ahora la pantalla se dibuja al principio (en lugar de cada vez que se presionaba Super), por lo que se aprecia mucho más firme la aparición de la ventana.</li>
	<li>0.10: Se reparó un bug donde la pantalla se dibujaba dos veces.</li>
	<li>0.10: Varios bugs reparados de estabilización, chorreos de memoria, inicialización, etc.</li>
	<li>0.3: El fallback de la versión 0.2 no funcionaba. Ahora se cae a la fuente que X11 recomiende (por lo general, Helvetica).</li>
	<li>0.2: Ya se hacen algunas pruebas de error básicas.</li>
	<li>0.2: Si no hay Bitstream Vera Sans, ahora se cae a Helvetica.</li>
	<li>0.1: Se añadió "make install" y "make uninstall" en Makefile.</li>
	<li>0.1: Se añadió un retardo para pintar el teclado. Se añadió la directiva DELAY.</li>
	<li>0.1: Mejor colocación de los iconos en secciones con rotación.</li>
	<li>0.1: Bicho muerto: No se ejecutaban los comandos establecidos en el programa de configuración.</li>
	<li>0.1: El nombre de alguna tecla se dibujaba parcialmente cuando había icono en dicha tecla.</li>
