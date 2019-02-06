import com.sun.org.apache.xpath.internal.compiler.OpCodes;
import org.objectweb.asm.*;
import org.objectweb.asm.util.TraceClassVisitor;

import java.io.*;
import java.util.Enumeration;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

/**
 * Created by Will_Zhang on 2016/6/16.
 */
public class ModifyMinecraftJava {
    public static void main(String[] args) {
        if(args.length == 0)
            return;

        try{
            ModifyMinecraftJava obj = new ModifyMinecraftJava();
            JarFile jarFile = new JarFile(args[0]);
            Enumeration<JarEntry> enu = jarFile.entries();
            while(enu.hasMoreElements()){
                JarEntry entry = enu.nextElement();
                if(entry.isDirectory()){
                    continue;
                }

                String name = entry.getName();
                if(name.compareTo("bao.class") == 0){
                    obj.modifyMinecraftClass("bao.class", jarFile.getInputStream(entry));
                }
                else if(name.compareTo("bjb.class") == 0){
                    obj.modifyNetHandlerClass("bjb.class", jarFile.getInputStream(entry));
                }
            }

        }catch(IOException e){
            e.printStackTrace();
        }
    }

    class NetHandlerTransform extends ClassVisitor {
        NetHandlerTransform(ClassVisitor cv) {
            super(Opcodes.ASM5, cv);
        }

        @Override
        public MethodVisitor visitMethod(int var1, String var2, String var3, String var4, String[] var5) {
            if (var2.compareTo("a") == 0 && var3.compareTo("(Lhd;)V") == 0)
                return new handleJoinGameMethodVisitor(super.visitMethod(var1, var2, var3, var4, var5));
            return super.visitMethod(var1, var2, var3, var4, var5);
        }
    }

    class handleJoinGameMethodVisitor extends MethodVisitor{
        public handleJoinGameMethodVisitor(MethodVisitor mv){
            super(Opcodes.ASM5, mv);
        }

        @Override
        public void visitInsn(int var1) {
            if(Opcodes.RETURN == var1){
                visitVarInsn(Opcodes.ALOAD, 0);
                visitFieldInsn(Opcodes.GETFIELD, "bjb", "f", "Lbao;");
                visitFieldInsn(Opcodes.GETFIELD, "bao", "craftBoxHelper", Type.getDescriptor(CraftBoxHelper.class));
                visitMethodInsn(Opcodes.INVOKEVIRTUAL, Type.getInternalName(CraftBoxHelper.class), "joinGame", "()V", false);
            }
            super.visitInsn(var1);
        }
    }

    class MinecraftClassTransform extends ClassVisitor{
        private boolean addCraftBoxType;

        MinecraftClassTransform(ClassVisitor cv){
            super(Opcodes.ASM5, cv);
            addCraftBoxType = false;
        }

        @Override
        public FieldVisitor visitField(int var1, String var2, String var3, String var4, Object var5) {
            if(!addCraftBoxType){
                addCraftBoxType = true;

                FieldVisitor visitor = super.visitField(Opcodes.ACC_PUBLIC, "craftBoxHelper", Type.getDescriptor(CraftBoxHelper.class), null, null);
                visitor.visitEnd();
            }

            return super.visitField(var1, var2, var3, var4, var5);
        }

        @Override
        public MethodVisitor visitMethod(int var1, String var2, String var3, String var4, String[] var5) {
            if(var2.compareTo("<init>") == 0)
                return new MainMethodVisitor(super.visitMethod(var1, var2, var3, var4, var5));
            else if(var2.compareTo("a")== 0 && var3.compareTo("(Lbjf;)V") == 0)
                return new LoadWorldMethodVisitor(super.visitMethod(var1, var2, var3, var4, var5));
            return super.visitMethod(var1, var2, var3, var4, var5);
        }
    }

    public void modifyMinecraftClass(String str, InputStream stream){
        try {
            ClassReader reader = new ClassReader(stream);
            ClassWriter writer = new ClassWriter(reader, ClassWriter.COMPUTE_FRAMES|ClassWriter.COMPUTE_MAXS);
            MinecraftClassTransform t = new MinecraftClassTransform(writer);
            TraceClassVisitor tracer = new TraceClassVisitor(new PrintWriter(System.out));
            reader.accept(t, ClassReader.EXPAND_FRAMES);
            FileOutputStream s = new FileOutputStream(str);
            s.write(writer.toByteArray());
            s.close();

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void modifyNetHandlerClass(String str, InputStream stream){
        try {
            ClassReader reader = new ClassReader(stream);
            ClassWriter writer = new ClassWriter(reader, ClassWriter.COMPUTE_FRAMES|ClassWriter.COMPUTE_MAXS);
            NetHandlerTransform t = new NetHandlerTransform(writer);
            TraceClassVisitor tracer = new TraceClassVisitor(new PrintWriter(System.out));
            reader.accept(t, ClassReader.EXPAND_FRAMES);
            FileOutputStream s = new FileOutputStream(str);
            s.write(writer.toByteArray());
            s.close();

        } catch (IOException e) {
            e.printStackTrace();
        }
    }



    class LoadWorldMethodVisitor extends MethodVisitor{
        public LoadWorldMethodVisitor(MethodVisitor mv){
            super(Opcodes.ASM5, mv);
        }

        @Override
        public void visitInsn(int var1) {
            if(Opcodes.RETURN == var1){
                Label ifNOTNull = new Label();
                visitVarInsn(Opcodes.ALOAD, 1);
                visitJumpInsn(Opcodes.IFNONNULL, ifNOTNull);

                visitVarInsn(Opcodes.ALOAD, 0);
                visitFieldInsn(Opcodes.GETFIELD, "bao", "craftBoxHelper", Type.getDescriptor(CraftBoxHelper.class));
                visitMethodInsn(Opcodes.INVOKEVIRTUAL , Type.getInternalName(CraftBoxHelper.class), "disconnectGame", "()V", false);

                visitLabel(ifNOTNull);
            }
            super.visitInsn(var1);
        }
    }

    class MainMethodVisitor extends MethodVisitor {
        public MainMethodVisitor(MethodVisitor mv) {
            super(Opcodes.ASM5, mv);
        }

        @Override
        public void visitInsn(int var1) {
            if (Opcodes.RETURN == var1) {
                visitVarInsn(Opcodes.ALOAD, 0);
                visitTypeInsn(Opcodes.NEW, Type.getInternalName(CraftBoxHelper.class));
                visitInsn(Opcodes.DUP);
                visitVarInsn(Opcodes.ALOAD, 0);
                visitMethodInsn(Opcodes.INVOKESPECIAL, Type.getInternalName(CraftBoxHelper.class), "<init>", "(Lbao;)V", false);
                visitFieldInsn(Opcodes.PUTFIELD, "bao", "craftBoxHelper", Type.getDescriptor(CraftBoxHelper.class));
            }
            super.visitInsn(var1);
        }
    }

}
