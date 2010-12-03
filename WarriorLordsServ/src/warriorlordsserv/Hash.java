package warriorlordsserv;

import java.io.UnsupportedEncodingException;
import java.security.*;

public class Hash
{
    private static String convertToHex(byte[] data)
    {
        StringBuffer buf = new StringBuffer();

        for (int i = 0; i < data.length; i++)
        {
            int halfbyte = (data[i] >>> 4) & 0x0F;
            int two_halfs = 0;
            do
            {
                if ((0 <= halfbyte) && (halfbyte <= 9))
                    buf.append((char) ('0' + halfbyte));
                else
                    buf.append((char) ('a' + (halfbyte - 10)));
                halfbyte = data[i] & 0x0F;
            }
            while(two_halfs++ < 1);
        }
        return buf.toString();
    }
 
    public static String SHA1(String text)
    {
		byte[] sha1hash = new byte[40];
		try
		{
			MessageDigest md;
			md = MessageDigest.getInstance("SHA-1");
			md.update(text.getBytes("iso-8859-1"), 0, text.length());
			sha1hash = md.digest();
		}
		catch(NoSuchAlgorithmException e)
		{
			return null;
		}
		catch(UnsupportedEncodingException e)
		{
			return null;
		}
		return convertToHex(sha1hash);
    }

    public static String MD5(String text)
    {
		byte[] md5hash = new byte[32];
		try
		{
			MessageDigest md;
			md = MessageDigest.getInstance("MD5");
			md.update(text.getBytes("iso-8859-1"), 0, text.length());
			md5hash = md.digest();
		}
		catch(NoSuchAlgorithmException e)
		{
			return null;
		}
		catch(UnsupportedEncodingException e)
		{
			return null;
		}
		return convertToHex(md5hash);
    }
}