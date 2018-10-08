package md5.server.endpoint;

import javax.xml.ws.Endpoint;
import md5.server.ws.MD5Impl;
import java.net.Inet4Address;

//Endpoint publisher
public class MD5Publisher{

	public static void main(String[] args) {
		String url = "http://";
		try{
			/* Get the machine's IP address in which the web service will be running */
			url += Inet4Address.getLocalHost().getHostAddress();
		}
		catch(Exception e){
			System.out.println("Error when getting IP address");
			return;
		}
		/* Build the web service URI */
		url += ":8080/ws/md5";
		System.out.println("Publishing MD5 service at endpoint: " + url);
		/* Publish the endpoint */
	    Endpoint.publish(url, new MD5Impl());
    }

}