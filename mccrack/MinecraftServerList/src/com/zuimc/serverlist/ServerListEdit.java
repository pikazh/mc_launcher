package com.zuimc.serverlist;

import org.jnbt.*;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.*;

/**
 * Created by Will_Zhang on 2016/6/25.
 */
public class ServerListEdit {
    public static void main(String[] args) {

        if (args.length < 2) {
            return;
        }

        NBTInputStream stream = null;
        List<Tag> new_server_list = new ArrayList<Tag>();

        Set<String> ips = new HashSet<>();

        String serverItems[] = args[1].split(",");
        for(String item : serverItems){
            item = item.trim();
            if(item.isEmpty())
                continue;

            String pair[] = item.split("\\|");
            if(pair.length != 2)
                continue;

            HashMap<String, Tag> entry_infos = new HashMap<String, Tag>();
            entry_infos.put("ip", new StringTag("ip", pair[0]));
            ips.add(pair[0]);
            entry_infos.put("name", new StringTag("name", pair[1]));
            entry_infos.put("hideAddress", new ByteTag("hideAddress", (byte) 0));
            CompoundTag add_server = new CompoundTag("", entry_infos);
            new_server_list.add(add_server);
        }


        try {
            stream = new NBTInputStream(new FileInputStream(args[0]), false);
            Tag t = stream.readTag();
            if (NBTUtils.getTypeCode(t.getClass()) == NBTConstants.TYPE_COMPOUND) {
                CompoundTag root = (CompoundTag) t;
                Map<String, Tag> vals = root.getValue();
                Iterator<Map.Entry<String, Tag>> entries = vals.entrySet().iterator();

                if (entries.hasNext()) {

                    Map.Entry<String, Tag> entry = entries.next();
                    if (entry.getKey().compareToIgnoreCase("servers") == 0 && NBTUtils.getTypeCode(entry.getValue().getClass()) == NBTConstants.TYPE_LIST) {

                        List<Tag> server_lists = ((ListTag) entry.getValue()).getValue();
                        Iterator<Tag> server_it = server_lists.iterator();

                        while (server_it.hasNext()) {
                            Tag s = server_it.next();
                            if (NBTUtils.getTypeCode(s.getClass()) == NBTConstants.TYPE_COMPOUND) {
                                CompoundTag server = (CompoundTag) s;
                                boolean need_add = true;
                                Iterator<Map.Entry<String, Tag>> server_info_it = server.getValue().entrySet().iterator();
                                while (server_info_it.hasNext()) {
                                    Map.Entry<String, Tag> info = server_info_it.next();
                                    if (NBTUtils.getTypeCode(info.getValue().getClass()) == NBTConstants.TYPE_STRING) {
                                        if (info.getKey().compareToIgnoreCase("ip") == 0 && ips.contains(info.getValue().getValue().toString())) {
                                            need_add = false;
                                            break;
                                        }
                                    }
                                }

                                if (need_add)
                                    new_server_list.add(s);
                            }
                        }
                    }

                }
            }

        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (stream != null)
                try {
                    stream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
        }


        ListTag list_tag = new ListTag("servers", CompoundTag.class, new_server_list);

        HashMap<String, Tag> server_map = new HashMap<String, Tag>();
        server_map.put("servers", list_tag);
        CompoundTag new_root = new CompoundTag("", server_map);
        NBTOutputStream out_stream = null;
        try {
            out_stream = new NBTOutputStream(new FileOutputStream(args[0]), false);
            out_stream.writeTag(new_root);
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (out_stream != null)
                    out_stream.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }


}
