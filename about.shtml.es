<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE html 
	 PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
	"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
  <head>
	<title>What & Why - Superkb</title>
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

	<div class ="index">
		<ul class="index">
			<li class="index"><a href="#what">¿Qué?</a>
			<li class="index"><a href="#why">¿Por qué?</a>
			<li class="index"><a href="#how">¿Cómo?</a>
			<li class="index"><a href="#weneed">Se necesita...</a>
			<li class="index"><a href="#who">¿Quién?</a>
			<li class="index"><a href="#license">Licencia y otras cosas</a>
		</ul>
	</div>

	<h1>Sobre Superkb</h1>

	<p>Esta página está dividida en varias secciones. Por favor use el menú
	de la derecha para navegar sobre ella.</p>

	<h2 id="what">¿Qué?</h2>

	<p>Superkb es un lanzador gráfico de programas
	para Linux. Funciona activándose al presionar una supertecla, normalmente
	Super_L o Super_R (mejor conocida como "la tecla de Windows"). Al activarse,
	presenta en pantalla un teclado con las teclas y sus correspondientes
	acciones.</p>

	<h3 id="guidelines">Pautas</h3>

	<p>Superkb debería:</p>

	<ul>
		<li>Nunca estorbarle al usuarios.</li>
		<li>Nunca desperdiciar espacio de pantalla.</li>
		<li>Ser ligero. Claro que esto es muy relativo.</li>
		<li>Nunca cambiar si el usuario no lo pide.</li>
		<li>Ser fácil de usar. También esto es relativo.</li>
	</ul>

	<p>Haremos lo mejor posible.</p>

	<h2 id="why">¿Por qué?</h2>

	<p><q>Es lo mismo que Windows.</q> &mdash;Mi amigo, decepcionado.</p>

	<p><q>Se ve igual que Windows.</q> &mdash;Mi hermana, decepcionada.</p>

	<p><q>¡Linux es muy difícil!</q> &mdash;Usuario al que se le muestra un Linux que no se parece a Windows.</p>

	<p><q>Bien. ¿qué hago ahora?</q> &mdash;Usuario primerizo (Win ó Lin) contemplando el escritorio vacío.</p>

	<p><q>¡Argh, ¿por qué no puedo abrir la calculadora más rápido?!</q> &mdash;Yo.</p>

	<p><q>¿Dónde hay una calculadora cuando la necesitas?</q> &mdash;Un usuario justo frente a su PC.</p>

	<p><q>No sabía que podía hacer eso...</q> &mdash;Un novato.</p>

	<p><q>¡¿Por qué no usar una maldita tecla para cargar la calculadora?!</q> &mdash;Yo.</p>

	<p><q>Mi teclado multimedia no trabaja en Linux.</q> &mdash;Caso hipotético.</p>

	<h2 id="how">¿Cómo?</h2>

	<p>El objetivo es repintar la tecla de la banderita con letras más grandes
	que digan "MENÚ" o algo similar. Por razones técnicas, me referiré a esa
	tecla como Super. Cada que presione cualquiera de las teclas Super,
	aparecerá un teclado en pantalla diciéndome qué teclas están enlazadas a
	qué, con sus respectivos iconos.</p>

    <p>Esto es a lo que me refiero: supongamos que estoy trabajando en algún
    programa y quiero rápidamente lanzar la calculadora. Primero, presiono (y
    mantengo) la tecla Super. Un teclado como el siguiente aparecerá en la
    pantalla:
    </p>

    <p><a href="shots/draft/draft-800-336.png">
      <img src="shots/draft/draft-640-269.png" alt="Teclado muestra" /></a>
    </p>

    <p>En este momento me doy cuenta de que la calculadora está en la "C", así
    que la presiono. Después de hacer esto varias veces, se debe convertir en
    un movimiento automático, incluso sin necesidad de que el teclado aparezca:
    sólo presionar Super+C en una reacción automática a mi deseo de correr la
    calculadora. (&iexcl;Intenta es con el mouse!)
    </p>

    <p>La imagen es sólo un bosquejo.</p>

    <p>Claro que habría algunos enlaces predefinidos. Por ejemplo:</p>

    <ul>
      <li>Super+Flechas: Movería la ventana actual 10px.</li>
      <li>Super+Alt+Flechas: Movería la ventana actual 1px.</li>
      <li>Super+Shift+Flechas: Cambiaría el tamaño de la ventana actual 10px de
        la esquina inferior derecha.</li>
      <li>Super+Alt+Shift+Flechas: Cambiaría de tamaño a la ventana actual 1px
        desde la esquina inferior derecha.</li>
      <li>Super+Shift+Insert: Correría un programa.</li>
      <li>Super+Shift+Delete: Matería (como xkill) la ventana activa.</li>
      <li>Super+Shift+End: Cerraría el programa activo.</li>
      <li>Super+Home: Regresaría al tamaño original (antes de maximizar).</li>
      <li>Super+PageUp: Maximizar.</li>
      <li>Super+PageDn: Minimizar.</li>
      <li>Super+N: Correría un programa de notas rápidas: (NEdit, GEdit, KEdit,
        XEdit, lo-que-sea-Edit)</li>
    </ul>

    <p>De acuerdo con el ejemplo, cuando el usuario presione la tecla Shift,
    &mdash;claro, mientras presiona la tecla Super &mdash;, superkb mostrará
    las acciones para Insert, Delete y End y otras teclas enlazadas a Shift
    y escondería aquellas que están solo enlazadas con presión natural.
    </p>

    <h2 id="details">Detalles</h2>
    <p>Así es como el programa deberá trabajar:</p>

    <ol>
      <li>Superkb carga &mdash;ya sea solo o como parte de un manejador de
        ventanas&mdash;.</li>
      <li>El usuario presiona y mantiene la tecla Super.</li>
      <li>Un teclado como el mostrado arriba aparece en la pantalla. Ese
        teclado deberá hacer notar al usuario que las teclas W, C y N hacen
        algo. También que las flechas y las teclas de edición hacen algo.</li>
      <li>El usuario podría hacer una de lo siguientes cosas:
        <ol>
          <li>El usuario presiona y suelta, digamos, la W. La acción apropiada
            (cargar Writer) deberá iniciarse. El teclado no deberá desaparecer
            de la pantalla.</li>
          <li>El usuario presiona (y mantiene) una tecla ya enlazada o
            enlazable, como la W, N, C u otra del alfabeto, pero no Pausa, por
            ejemplo. Si el usuario mantiene la tecla presionada durante 3
            segundos, el teclado debe desaparecer y la ventana de
            "Configuración de Tecla" debe aparecer. Si el usuario suelta la
            tecla antes de lso 3 segundos, en realidad es el punto 4.1.</li>
          <li>Un usuario podría usar el ratón para mover o descansar el puntero
            sobre una tecla:
            <ol>
              <li>Si pasa el puntero sobre una tecla enlazada, deberá aparecer
                información adicional &mdash;como hacer el icono más grande&mdash;.</li>
              <li>Si el usuario hace clic sobre una tecla enlazada, la acción 
                deberá llevarse a cabo.</li>
              <li>Si el usuario presiona y mantiene un botón sobre una tecla
                deberá ser configurada según el punto 4.2.</li>
            </ol>
          </li>
        </ol>
      </li>
      <li>Después de que el usuario haya cargado todos los programas que desea
        soltará la tecla y el teclado desaparecerá. Los programas deberían en
        este momento estar cargándose ya.</li>
    </ol>

	<h2 id="weneed">Se necesita</h2>

    <ol>
      <li>Retroalimentación sobre la compilación de Superkb. La idea es
        que funcione sin necesidad de ./configure tanto como se pueda.</li>
      <li>Una rutina para determinar el lugar óptimo para poner el icono en
        teclas poligonales (como los teclados multimedia y ENTER en forma de
        L.</li>
      <li>Portar la librería cargaiconos (puticon_gdk_pixbuf_xlib.c) a Qt.</li>
      <li>Código para rotar fuentes (realmente no se puede con Xlib?)</li>
      <li>Muchas otras cosas.</li>
    </ol>

	<h2 id="who">Who?</h2>

    <p>Mi nombre es Octavio Alvarez. Mi dirección de correo electrónico es
      alvarezp@alvarezp.ods.org. También estoy (a veces) en irc.freenode.net
      en #superkb como alvarezp u octal.</p>

	<h2 id="license">License and Legal Stuff</h2>

    <p>Todo el software relacionado con este proyecto, incluidos los prototipos
    y bosquejos están distribuidos bajo los términos de la <a href="http://www.gnu.org/copyleft/gpl.html">Licencia Pública General, versión 2</a>. Todos el arte está bajo
    los términos de <a rel="license"
    href="http://creativecommons.org/licenses/by-sa/2.5/">Creative Commons Attribution-ShareAlike 2.5 License</a>.</p>

    <p>
    El teclado es una versión modificada <a href="http://openclipart.org/clipart//computer/hardware/Computer-Keyboardlayout-de.svg">
    del que puso Christoph Eckert en el dominio público</a>.
    </p>

    <p>Aquí hay un enlace a <a href="http://www.sourceforge.net/projects/superkb">la página del proyecto Superkb en Sourceforge.net</a>.

    <p><a href="http://sourceforge.net"><img style="float: left; margin-right: 10px;"
    src="http://sflogo.sourceforge.net/sflogo.php?group_id=154661&amp;type=2" width="125"
    height="37" alt="SourceForge.net Logo" /></a>También, según lo piden las
    políticas de SourceForge, aquí está el logo de SourceForge.net, el gran
    anfitrión de proyectos open source, que de este modo ayuda mucho al
    proyecto Superkb.</p>

	</div>

  </body>
</html>


