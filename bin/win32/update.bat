
copy /B /Y ..\..\src\CoreModules\build\CGCP.exe .
copy /B /Y ..\..\src\CoreModules\build\modules\ParserSotp.dll modules\
copy /B /Y ..\..\src\CoreModules\build\modules\ParserHttp.dll modules\
copy /B /Y ..\..\src\CoreModules\build\modules\LogService.dll modules\
copy /B /Y ..\..\src\CoreModules\build\modules\SotpClientService.dll modules\
#copy /B /Y ..\..\src\CGCComms\build\CommRtpServer.dll modules\
copy /B /Y ..\..\src\CGCComms\build\CommTcpServer.dll modules\
copy /B /Y ..\..\src\CGCComms\build\CommUdpServer.dll modules\
copy /B /Y ..\..\src\CGCServices\build\ConfigurationService.dll modules\
copy /B /Y ..\..\src\CGCServices\build\StringService.dll modules\
copy /B /Y ..\..\src\CGCServices\build\FileSystemService.dll modules\
#copy /B /Y ..\..\src\CGCServices\build\RtpService.dll modules\
copy /B /Y ..\..\src\CGCServices\build\BodbService.dll modules\
copy /B /Y ..\..\src\CGCServices\build\NamespaceService.dll modules\
copy /B /Y ..\..\src\CGCServices\build\MysqlService.dll modules\
copy /B /Y ..\..\src\CGCServices\build\XmlService.dll modules\
copy /B /Y ..\..\src\CGCServices\build\HttpService.dll modules\
copy /B /Y ..\..\src\CGCServices\build\DateTimeService.dll modules\
#copy /B /Y ..\..\src\CGCServices\build\PGService.dll modules\
copy /B /Y ..\..\src\Projects\build\HttpServer.dll modules\
#copy /B /Y ..\..\src\Projects\build\MVCServer.dll modules\

copy /B /Y ..\..\samples\build\cspServlet.dll modules\
copy /B /Y ..\..\samples\build\RestfulTest.dll modules\
copy /B /Y ..\..\samples\build\cspApp.dll modules\

copy /B /Y ..\..\samples\build\DLLTestClient.exe .
copy /B /Y ..\..\samples\build\DLLTest.dll modules\

@pause
