package warriorlordsserv;

import java.io.UnsupportedEncodingException;
import java.util.Properties;
import javax.mail.Message;
import javax.mail.MessagingException;
import javax.mail.Session;
import javax.mail.Transport;
import javax.mail.internet.AddressException;
import javax.mail.internet.InternetAddress;
import javax.mail.internet.MimeMessage;

public class EmailService
{
	public static boolean SendEmail(String senderAddress, String sender, String recipientAddress, String recipient, String subject, String message, String ccAddress, String cc)
	{
        Properties props = new Properties();
        Session session = Session.getDefaultInstance(props, null);

        try
		{
			Message msg = new MimeMessage(session);

			if(sender != null)
				msg.setFrom(new InternetAddress(senderAddress, sender));
			else
				msg.setFrom(new InternetAddress(senderAddress));

			if(recipient != null)
				msg.addRecipient(Message.RecipientType.TO, new InternetAddress(recipientAddress, recipient));
			else
				msg.addRecipient(Message.RecipientType.TO, new InternetAddress(recipientAddress));

			if(ccAddress != null)
			{
				if(cc != null)
					msg.addRecipient(Message.RecipientType.CC, new InternetAddress(ccAddress, cc));
				else
					msg.addRecipient(Message.RecipientType.CC, new InternetAddress(ccAddress));
			}

			msg.setSubject(subject);
			msg.setText(message);

			Transport.send(msg);
		}
		catch(AddressException e)
		{
			return false;
		}
		catch(MessagingException e)
		{
			return false;
		}
		catch(UnsupportedEncodingException e)
		{
			return false;
		}
		
		return true;
	}

	public static boolean SendEmail(String senderAddress, String sender, String recipientAddress, String recipient, String subject, String message)
	{
		return SendEmail(senderAddress, sender, recipientAddress, recipient, subject, message, null, null);
	}

	public static boolean SendEmail(String senderAddress, String recipientAddress, String subject, String message)
	{
		return SendEmail(senderAddress, null, recipientAddress, null, subject, message, null, null);
	}
}
