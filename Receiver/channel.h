const char channel_html[] PROGMEM = R"=====(<!DOCTYPE html>
<!DOCTYPE html>
<html>
<head>
  <title>Shot-Clock-Channel</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
  html {
    font-family: Arial, Helvetica, sans-serif;
    text-align: center;
  }
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2{
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
    overflow: hidden;
    background-color: #143642;
  }
  body {
    margin: 0;
  }
  .content {
    padding: 30px;
    max-width: 600px;
  //  margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }
  .link-as-button {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #0f8b8d;
    border: none;
    border-radius: 5px;
    -webkit-touch-callout: none;
    -webkit-user-select: none;
    -khtml-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;
    user-select: none;
    -webkit-tap-highlight-color: rgba(0,0,0,0);
   }
   /*.button:hover {background-color: #0f8b8d}*/
   .link-as-button:active {
     background-color: #0f8b8d;
     box-shadow: 2 2px #CDCDCD;
     transform: translateY(2px);
   }
   .state {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
   }
  </style>
<title>Shot-Clock-Channel</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <h1>Shot-Clock-Channel</h1>
  </div>
  <div class="content">
    <div class="card">
      <a class="link-as-button" href="/1">Channel 1</a>
      <a class="link-as-button" href="/2">Channel 2</a>
      <a class="link-as-button" href="/3">Channel 3</a>
      <a class="link-as-button" href="/4">Channel 4</a>
      <a class="link-as-button" href="/5">Channel 5</a>
      <a class="link-as-button" href="/6">Channel 6</a>
      <a class="link-as-button" href="/7">Channel 7</a>
      <a class="link-as-button" href="/8">Channel 8</a>    
    </div>
  </div>
<script>
</script>
</body>
</html>
)=====";
