<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
		"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="es" lang="es">
<head>
	<meta name="generator" content=
	"HTML Tidy for Linux/x86 (vers 1 September 2005), see www.w3.org" />

	<title>Documentación - Superkb</title>
        <!--#include virtual="stylesheets.html" -->
	<meta http-equiv="Content-type" content=
	"application/xhtml+xml; charset=utf-8" />
<style type="text/css">
/*<![CDATA[*/
<!--
																-->
/*]]>*/
</style>
  <!--#include virtual="analytics.html" -->
</head>

<body>
	<!--#include virtual="menu.html.es" -->

	<div class="content">
		<div class="index">
			<ul class="index">
				<li class="index"><a href="#source">Código fuente</a></li>

				<li class="index"><a href="#compilation">Compilación</a></li>

				<li class="index"><a href="#installation">Instalación</a></li>

				<li class="index"><a href="#configuration">Configuración</a>
				</li>

			</ul>
		</div>

		<h1>Algo de documentación</h1>

		<p>Esta página está dividida en varias secciones. Use el menú flotante
		de la derecha para navegar en ella.</p>

		<h2 id="source">Código fuente</h2>

		<p>El código fuente actualmente está en Git. Para obtenerlo,
		es necesario contar con una correcta instalación de
		Git. Después, use:</p>

		<p><code>git clone git://superkb.git.sourceforge.net/gitroot/superkb/superkb superkb-git</code></p>

		<p>También, el código tiene muchos FIXME ("arréglame") and necesitan
		trabajo. Están documentados a lo largo del código.</p>

		<h2 id="compilation">Compilación</h2>

		<p>Un simple <code>make</code> debe bastar. Sin embargo, en algunas
		sdistribuciones es necesario aplicarlo dos veces.</p>

		<p>Superkb usa un archivo llamado "configuration" para registrar las
		cualidades actuales del sistema. Este archivo acepta las siguientes
		directivas:</p>

		<ul>
			<li><strong>PUTICON_GDKPIXBUF={y|m|n}</strong>: indica si se debe
			compilar el soporte para Gdk-Pixbuf (y), incluirlo como módulo (m) o
			no incluirlo de plano (n). Normalmente lo detecta Makefile mediante
			<code>pkg-config gdk-pixbuf-xlib-2.0</code>.</li>

			<li><strong>PUTICON_IMLIB2={y|m|n}</strong>: indica si se debe
			compilar el soporte para Imlib2 (y), incluirlo como módulo (m) o no
			incluirlo de plano (n). Normalmente lo detecta Makefile mediante
			<code>imlib2-config</code>.</li>
		</ul>

		<h2 id="installation">Instalación</h2>

		<p>Un simple <code>make install</code> debe bastar. Las rutas de
		instalación son fijas en este momento.</p>

		<h2 id="configuration">Configuración</h2>

		<p>Superkb busca un archivo llamado .superkbrc en el directorio $HOME.
		El archivo es interpretado como un script, por lo que si dos líneas se
		contradicen, gana la segunda en orden de aparición. Se pueden indicar
		comentarios mediante el signo de número (#) y se ignora el resto de la
		lí­nea. La sintaxis básica es "CLAVE valor", pero si el valor incluye
		espacios, se debe envolver en comillas ("). Las directivas que se
		admiten son:</p>

		<ul>
			<li><strong>IMAGELIB {gdkpixbuf|imbli2}</strong>: Establece la
			librerí­a para cargar iconos. Se debe incluir soporte ya sea
			compilado o como módulo. Vea <a href="#compilation">Compilación</a>
			para mayor información. Valor por omisión: "imlib2".</li>

			<li><strong>FONT "fuente"</strong>: El nombre XLFD de la fuente a
			usar. Recomiendo usar una fuente escalable. Yo uso <code>xfontsel
			-pattern "*-0-0-0-0-*-0-*"</code> para cargar un selector de fuentes
			apropiadas. Sin embargo, algunas personas han indicado que esto no
			necesariamente funciona. No olvide las comillas en el nombre de la
			fuente, ya que algunas incluyen espacios. Valor por omisión:
			"-*-bitstream vera sans-bold-r-*-*-*-*-*-*-*-*-*-*".</li>

			<li><strong>BACKGROUND r v a</strong>: Marca el color de fondo a
			usar. Cada valor debe estar entre 0 y 65535. Muy probablemente esto
			cambiará en un futuro. Valor por omisión: 59500 59500 59500. (Sí,
			está feo.)</li>

			<li><strong>FOREGROUND r v a</strong>: Marca el color de primer
			plano a usar. Cada valor debe estar entre 0 y 65535. Muy
			probablemente esto cambiará en un futuro. Valor por omisión: 2000
			2000 2000. (Je, también está feo.)</li>

			<li><strong>DELAY segundos</strong>: Marca el tiempo a esperar a
			mostrar el teclado después de que se mantiene la tecla de Super
			presionada. Se admiten valores decimales. Valor por omisión: 0.5.
			</li>
			<li><strong>SUPERKEY1_STRING</strong>: Establece la primera tecla
			Super, por nombre. Valor por omisión: "Super_L"
			</li>
			<li><strong>SUPERKEY2_STRING</strong>: Establece la segunda tecla
			Super, por nombre. Valor por omisión: "Super_R"
			</li>
			<li><strong>SUPERKEY1_CODE</strong>: Establece la primera tecla
			Super, por código de tecla de X.
			</li>
			<li><strong>SUPERKEY1_STRING</strong>: Establece la segunda tecla
			Super, por por código de tecla de X.
			</li>
			<li><strong>KEY COMMAND tecla máscara comando icono [retro]</strong>:
			Ejecuta un comando arbitrario.
			"Tecla" indica la tecla a usar. No todas las teclas funcionan,
			en este momento, pero sí las letras y las de función (como "F1").
			"Máscara" está reservado para uso futuro; sólo hay que poner un 0.
			"Comando" es el comando a ejecutar (no olvidar poner las comillas
			si se necesita) e "Icono" es la ubicación ABSOLUTA del archivo con
			el icono a mostrar en el teclado. La cadena opcional "retro" debe aparecer con la línea a añadir
			al FEEDBACK_HANDLER. Por ejemplo, si el FEEDBACK_HANDLER es 'xmessage -timeout 2 -center Cargando ', la cadena "retro"
			debe ser algo como "gedit", para que muestre "Cargando gedit" en pantalla.
			Nótese que omitir la cadena "retro" deshabilita la retroalimentación para esa KEY.</li>

			<li><strong>KEY DOCUMENT tecla máscara documento icono [retro]</strong>:
			Usa DOCUMENT_HANDLER para abrir el documento especificado
			"Tecla" indica la tecla a usar. No todas las teclas funcionan,
			en este momento, pero sí las letras y las de función (como "F1").
			"Máscara" está reservado para uso futuro; sólo hay que poner un 0.
			"Comando" es el comando a ejecutar (no olvidar poner las comillas
			si se necesita) e "Icono" es la ubicación ABSOLUTA del archivo con
			el icono a mostrar en el teclado. La cadena opcional "retro" debe aparecer con la línea a añadir
			al FEEDBACK_HANDLER. Por ejemplo, si el FEEDBACK_HANDLER es 'xmessage -timeout 2 -center Cargando ', la cadena "retro"
			debe ser algo como "gedit", para que muestre "Cargando gedit" en pantalla.
			Nótese que omitir la cadena "retro" deshabilita la retroalimentación para esa KEY.</li>

			<li><strong>DOCUMENT_HANDLER cadena</strong>: Indica el programa a usar para
			abrir los documentos indicados por una KEY DOCUMENT.
			</li>

			<li><strong>FEEDBACK_HANDLER cadena</strong>: Indica el programa a usar para
			mandar el feedback cuando se ejecuta una KEY. Técnicamente hablando, el comando
			se compone del parámetro "retro" en el comando KEY, añadido al parámetro
			indicado en FEEDBACK_HANDLER.
			</li>

			<li><strong>SUPERKEY_REPLAY { 0 | 1 }</strong>: Indica si
			la tecla Super debe ser reenviada a la pantalla de la cual
			fue robada, de modo que no se pierda el uso convencional de
			la tecla. Por ejemplo, si se usa F10 como tecla Super,
			esta opción permite que si la tecla se presiona sin combinarse
			con otra ni permitir que el teclado aparezca en pantalla (estilo
			click), se reenvíe ese F10 a la aplicación original, de modo que
			en GNOME, se presentaría el menú en las aplicaciones que tienen
			uno.
			</li>
		</ul>

</body>
</html>
