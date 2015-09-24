// ===============================
//  PC-BSD REST API Server
// Available under the 3-clause BSD License
// Written by: Ken Moore <ken@pcbsd.org> July 2015
// =================================
#include "WebServer.h"

#include <QCoreApplication>
#include <QUrl>


#define DEBUG 0

#define PORTNUMBER 12142

//=======================
//              PUBLIC
//=======================
WebServer::WebServer() : QWebSocketServer("syscache-webclient", QWebSocketServer::NonSecureMode){
  //Setup all the various settings
  //Any SSL changes
    /*QSslConfiguration ssl = this->sslConfiguration();
      ssl.setProtocol(QSsl::SecureProtocols);
    this->setSslConfiguration(ssl);*/

  //Setup Connections
  connect(this, SIGNAL(closed()), this, SLOT(ServerClosed()) );
  connect(this, SIGNAL(serverError(QWebSocketProtocol::CloseCode)), this, SLOT(ServerError(QWebSocketProtocol::CloseCode)) );
  connect(this, SIGNAL(newConnection()), this, SLOT(NewSocketConnection()) );
  connect(this, SIGNAL(acceptError(QAbstractSocket::SocketError)), this, SLOT(NewConnectError(QAbstractSocket::SocketError)) );
  connect(this, SIGNAL(originAuthenticationRequired(QWebSocketCorsAuthenticator*)), this, SLOT(OriginAuthRequired(QWebSocketCorsAuthenticator*)) );
  connect(this, SIGNAL(peerVerifyError(const QSslError&)), this, SLOT(PeerVerifyError(const QSslError&)) );
  connect(this, SIGNAL(sslErrors(const QList<QSslError>&)), this, SLOT(SslErrors(const QList<QSslError>&)) );
}

WebServer::~WebServer(){
}

bool WebServer::startServer(){
  bool ok = this->listen(QHostAddress::Any, PORTNUMBER);
  if(ok){ 
    QCoreApplication::processEvents();
    qDebug() << "Server Started:" << QDateTime::currentDateTime().toString(Qt::ISODate);
    qDebug() << " Name:" << this->serverName() << "Port:" << this->serverPort();
    qDebug() << " URL:" << this->serverUrl().toString() << "Remote Address:" << this->serverAddress().toString();
  }else{ qCritical() << "Could not start server - exiting..."; }
  return ok;
}

void WebServer::stopServer(){
  this->close();
}

//===================
//     PRIVATE
//===================
QString WebServer::generateID(){
  int id = 0;
  for(int i=0; i<OpenSockets.length(); i++){
    if(OpenSockets[i]->ID().toInt()>=id){ id = OpenSockets[i]->ID().toInt()+1; }
  }
  return QString::number(id);
}

//=======================
//       PRIVATE SLOTS
//=======================
// Overall Server signals
void WebServer::ServerClosed(){
  qDebug() << "Server Closed:" << QDateTime::currentDateTime().toString(Qt::ISODate);
  QCoreApplication::exit(0);
}

void WebServer::ServerError(QWebSocketProtocol::CloseCode code){
  qWarning() << "Server Error["+QString::number(code)+"]:" << this->errorString();
}

// New Connection Signals
void WebServer::NewSocketConnection(){
  if(!this->hasPendingConnections()){ return; }
  qDebug() << "New Socket Connection";	
  //if(idletimer->isActive()){ idletimer->stop(); }
  QWebSocket *csock = this->nextPendingConnection();
  if(csock == 0){ qWarning() << " - new connection invalid, skipping..."; QTimer::singleShot(10, this, SLOT(NewSocketConnection())); return; }
  qDebug() <<  " - Accepting connection:" << csock->origin();
  WebSocket *sock = new WebSocket(csock, generateID());
  connect(sock, SIGNAL(SocketClosed(QString)), this, SLOT(SocketClosed(QString)) );
  OpenSockets << sock;
}

void WebServer::NewConnectError(QAbstractSocket::SocketError err){
  //if(csock!=0){
    //qWarning() << "New Connection Error["+QString::number(err)+"]:" << csock->errorString();
    //csock->close();
  //}else{            
    qWarning() << "New Connection Error["+QString::number(err)+"]:" << this->errorString();
  //}
  //csock = 0; //remove the current socket
  QTimer::singleShot(0,this, SLOT(NewSocketConnection()) ); //check for a new connection
  
}

// SSL/Authentication Signals
void WebServer::OriginAuthRequired(QWebSocketCorsAuthenticator *auth){
  qDebug() << "Origin Auth Required:" << auth->origin();
  //if(auth->origin() == this->serverAddress().toString()){
  // TO-DO: Provide some kind of address filtering routine for which to accept/reject
    qDebug() << " - Allowed";
    auth->setAllowed(true);
  //}else{
    //qDebug() << " - Not Allowed";
    //auth->setAllowed(false);
  //}
	
}

void WebServer::PeerVerifyError(const QSslError &err){
  qDebug() << "Peer Verification Error:" << err.errorString();
	
}

void WebServer::SslErrors(const QList<QSslError> &list){
  qWarning() << "SSL Errors:";
  for(int i=0; i<list.length(); i++){
    qWarning() << " - " << list[i].errorString();
  }
}

void WebServer::SocketClosed(QString ID){
  for(int i=0; i<OpenSockets.length(); i++){
    if(OpenSockets[i]->ID()==ID){ delete OpenSockets.takeAt(i); break; }
  }
  QTimer::singleShot(0,this, SLOT(NewSocketConnection()) ); //check for a new connection
}