<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE html 
     PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
  <head>
    <title>Superkb</title>
        <!--#include virtual="stylesheets.html" -->
    <meta http-equiv="Content-type"
      content="application/xhtml+xml; charset=utf-8" />
    <style type="text/css"><!--
    --></style>
  <!--#include virtual="analytics.html" -->
  </head>
  <body> 

    <!--#include virtual="menu.html.es" -->

	<div class="content">

    <h1>Descarga</h1>

    <p>Superkb está aún en pañales, por lo que aún no hay paquetes binarios a
      disposición. Sin embargo, hay tres maneras de descargar el código
      fuente de Superkb:</p>
    <ul>
      <li>Descargar <a href="#package">el paquete más reciente</a>, ya sea en código fuente o en binario precompilado para Ubuntu.</li>
      <li>Descargar <a href="#get-git">el código actual desde Git</a>.</li>
      <li>Navegar por el repositorio de Git en línea. <a href="#browse-git">(Instrucciones)</a></li>
    </ul>

    <h2 id="package">Paquetes más reciente</h2>

    <p id='current-tarball-version'>La versión actual es: <!--#include virtual="CURRENT_VERSION" -->, liberada en <!--#include virtual="CURRENT_VERSION_RELEASE_DATE" -->.</p>

    <ul>
      <li id='download-current-tarball-gz'>Tarbola actual en gz: <a href='http://prdownloads.sourceforge.net/superkb/superkb-<!--#include virtual="CURRENT_VERSION" -->.tar.gz?download'>superkb-<!--#include virtual="CURRENT_VERSION" -->.tar.gz</a></li>
     <li id='download-current-tarball-bz2'>Tarbola actual en bz2: <a href='http://prdownloads.sourceforge.net/superkb/superkb-<!--#include virtual="CURRENT_VERSION" -->.tar.bz2?download'>superkb-<!--#include virtual="CURRENT_VERSION" -->.tar.bz2</a></li>
     <li id='download-current-binary-fc12'>Binario actual para Fedora Core 12: <a href='http://prdownloads.sourceforge.net/superkb/superkb-<!--#include virtual="CURRENT_VERSION" -->-1.fc12.i686.rpm?download'>superkb-<!--#include virtual="CURRENT_VERSION" -->-1.fc12.i686.rpm</a></li>
    </ul>

    <h3>Bitácora de cambios (desde -proto3):</h3>
    <ul id="download-full-changelog">
    <!--#include virtual="changelog.html.part.es" -->
    </ul>

    <h3>Algunas notas:</h3>
    <ul>
      <li>Hay una muestra del archivo de configuración en ./sample-config.</li>
      <li>Superkb depende de Xkb y --ya sea-- gdk-pixbuf/pkg-config o imlib2.</li>
      <li>Se necesita una fuente escalable. Se recomienda buscar una con
		<code>xfontsel -pattern "*-0-0-0-0-*-0-*"</code></li>
      <li>Para compilar hay que dar "make" desde el directorio "src". Superkb
		debe detectar si se cuenta con imlib2 o gdk-pixbuf.</li>
    </ul>

    <h2 id="get-git">Descargar las fuentes actuales</h2>

    <p>La versión de desarrollo está en Git, que puedes descargar mediante:
    </p>

    <p><code>git clone git://superkb.git.sourceforge.net/gitroot/superkb/superkb superkb-git</code></p>

    <h2 id="browse-git">Navegando por el repositorio Git</h2>

    <p>Como una alternativa a bajar el código completo, puedes apuntar tu
    navegador hacia <a href="http://superkb.git.sourceforge.net/">
    y ver los archivos individuales mediante la herramienta Gitweb de SF.</p>

	</div>

  </body>
</html>


