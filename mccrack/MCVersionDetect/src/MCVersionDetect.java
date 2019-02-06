import org.objectweb.asm.ClassReader;

import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

/**
 * Created by Will_Zhang on 2016/6/15.
 */
public class MCVersionDetect {
    public static void main(String[] args) {
        if(args.length == 0)
            return;

        MCVersionDetect detect = new MCVersionDetect();
        try {
            JarFile jarFile = new JarFile(args[0]);
            Enumeration<JarEntry> enu = jarFile.entries();
            while(enu.hasMoreElements()){
                JarEntry entry = enu.nextElement();
                if(entry.isDirectory())
                    continue;

                String name = entry.getName();
                if(name.compareTo("ave.class") == 0 && detect.getVersion(jarFile.getInputStream(entry))){
                    return;
                }
                if(name.compareTo("avf.class") == 0 && detect.getVersion(jarFile.getInputStream(entry))){
                    return;
                }
                if(name.compareTo("bss.class") == 0 && detect.getVersion(jarFile.getInputStream(entry))){
                    return;
                }
                if(name.compareTo("bsu.class") == 0 && detect.getVersion(jarFile.getInputStream(entry))){
                    return;
                }
                if(name.compareTo("bao.class") == 0 && detect.getVersion(jarFile.getInputStream(entry))){
                    return;
                }
                if(name.compareTo("azd.class") == 0 && detect.getVersion(jarFile.getInputStream(entry))){
                    return;
                }
                if(name.compareTo("bcf.class") == 0 && detect.getVersion(jarFile.getInputStream(entry))){
                    return;
                }
                if(name.compareTo("bcc.class") == 0 && detect.getVersion(jarFile.getInputStream(entry))){
                    return;
                }
                if(name.compareTo("bcd.class") == 0 && detect.getVersion(jarFile.getInputStream(entry))){
                    return;
                }
                if(name.compareTo("bcx.class") == 0 && detect.getVersion(jarFile.getInputStream(entry))){
                    return;
                }
                if(name.compareTo("ats.class") == 0 && detect.getVersion(jarFile.getInputStream(entry))){
                    return;
                }
                if(name.compareTo("atv.class") == 0 && detect.getVersion(jarFile.getInputStream(entry))){
                    return;
                }
            }
        } catch (IOException e) {
        }
    }

    public boolean getVersion(InputStream stream){
        try {
            ClassReader reader = new ClassReader(stream);
            int constantItemCount = reader.getItemCount();
            char[] buffer = new char[reader.getMaxStringLength()+1];
            for(int i = 1;i<constantItemCount;i++){
                int pos = reader.getItem(i);
                if(pos == 0 || reader.b[pos-1] != 8)
                    continue;

                Arrays.fill(buffer, (char)0);
                String str = reader.readUTF8(pos, buffer);
                if(str.startsWith("Minecraft ")){
                    System.out.print(str);
                    return true;
                }
            }
        } catch (IOException e) {
            return false;
        }

        return false;
    }
}
