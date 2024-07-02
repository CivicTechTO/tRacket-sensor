const char *HTML_HEADER = R"html(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>tRacket</title>
<style>
body {background-color: #fff4e6;font-family: Arial, Helvetica, sans-serif;}
*{padding: 0;margin: 0;}
main{padding: 20px 0px 0px 0px;padding-bottom: 2.5rem;    /* Footer height */}
#page-container {position: relative;min-height: 100vh;}
header{height: 76px;background-color: #242424;color: #fff;}
header brand h2{padding: 25px 0px 0px 10%;}
section{padding: 25px 10% 0px;}
footer {position: absolute;bottom: 0;width: 100%;display: flex;padding-top: 10px;justify-content: center;height: 2.5rem;font-size: 20px;background-color: #000;color: #fff;}
input[type=submit]{
font-size: 24px;
padding: 5px;
margin-top: 3px;
}
.meter { 
    height: 5px;
    position: relative;
    background: #f3efe6;
    overflow: hidden;
    width: 90%;
}
.meter span {
    display: block;
    height: 100%;
}
.progress {
    background-color: #e4c465;
    -webkit-animation: progressBar 30s ease-in-out;
    -webkit-animation-fill-mode:both; 
    -moz-animation: progressBar 30s ease-in-out;
    -moz-animation-fill-mode:both; 
}
@-webkit-keyframes progressBar {
  0% { width: 0; }
  100% { width: 100%; }
}
@-moz-keyframes progressBar {
  0% { width: 0; }
  100% { width: 100%; }
}
</style>
</head>
<body>
<div id="page-container">
<header><nav><brand><h2>tRacket</h2></brand></nav></header>
<main>
<section>
<div class="Container">
<h1>tRacket sensor setup</h1>
)html";

const char *HTML_FOOTER = R"html(
</div> 
</section>

</main>
<footer>
<p>info@tRacket.info</p>
</footer>
</div>
</body>
</html>
)html";

const char *HTML_BODY_FORM = R"html(
<p>Enter the wifi network name and password for your home network, which the sensor can connect to to get online:<br/><br/></p>
    <form method='POST' action='' enctype='multipart/form-data'>
      <p>Wifi network name:</p>
      <input type='text' name='ssid' required>
      <p>Wifi network password:</p>
      <input type='password' name='psk'>
      <p>Your Email (also your username for logging into the tRacket portal):</p>
      <input type='email' name='email'>
      <p><input type='submit' value='Connect'></p>
    </form>
)html";

