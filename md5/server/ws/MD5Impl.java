// Based on http://www.mkyong.com/java/java-md5-hashing-example/
package md5.server.ws;

import java.io.FileInputStream;
import java.security.MessageDigest;
import javax.jws.WebService;

//Service Implementation
@WebService(endpointInterface = "md5.server.ws.MD5")
public class MD5Impl implements MD5
{
   /**
    * Performs the MD5 algorithm in order to produce a 128-bit hash value.
    * @param text input text argument.
    * @return The calculated MD5 hash value (in hex format).
    */
    @Override
    public String getMD5(String text) throws Exception
    {
    // Calculate MD5(text)
        MessageDigest md = MessageDigest.getInstance("MD5");
        byte[] dataBytes = text.getBytes();
        md.update(dataBytes, 0, text.length());
        byte[] mdbytes = md.digest();

        // Convert byte to hex format
        StringBuffer sb = new StringBuffer();
        for (int i = 0; i < mdbytes.length; i++) {
          sb.append(Integer.toString((mdbytes[i] & 0xff) + 0x100, 16).substring(1));
        }

    return sb.toString();
    }
}
