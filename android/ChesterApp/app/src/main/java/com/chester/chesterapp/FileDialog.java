package com.chester.chesterapp;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Environment;
import android.support.v7.app.AlertDialog;

import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

// Lots of the credit for this code goes to schwiz and others who
// contributed to this helper class on Stack Overflow, see
// https://stackoverflow.com/questions/3592717/choose-file-dialog

public class FileDialog {
    private static final String PARENT_DIR = "..";
    private String[] mFileList;
    private File mCurrentPath;
    public interface FileSelectedListener {
        void fileSelected(File file);
    }
    private ListenerList<FileSelectedListener> mFileListenerList = new ListenerList<>();
    private final Context mContext;
    private String mFileEnd;

    FileDialog(Context context, File initialPath, String fileEndsWith) {
        this.mContext = context;
        setFileEndsWith(fileEndsWith);
        if (!initialPath.exists()) initialPath = Environment.getExternalStorageDirectory();
        loadFileList(initialPath);
    }

    private Dialog createFileDialog() {
        Dialog dialog;
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(mCurrentPath.getPath());
        builder.setItems(mFileList, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                String fileChosen = mFileList[which];
                File chosenFile = getChosenFile(fileChosen);
                if (chosenFile.isDirectory()) {
                    loadFileList(chosenFile);
                    dialog.cancel();
                    dialog.dismiss();
                    showDialog();
                } else {
                    fileSelected(chosenFile);
                }
            }
        });

        dialog = builder.show();
        dialog.setCancelable(false);
        return dialog;
    }

    public void addFileListener(FileSelectedListener listener) {
        mFileListenerList.add(listener);
    }

    public void showDialog() {
        createFileDialog().show();
    }

    private void fileSelected(final File file) {
        mFileListenerList.fireEvent(new ListenerList.FireHandler<FileSelectedListener>() {
            public void fireEvent(FileSelectedListener listener) {
                listener.fileSelected(file);
            }
        });
    }

    private void loadFileList(File path) {
        this.mCurrentPath = path;
        List<String> r = new ArrayList<>();
        if (path.exists()) {
            if (path.getParentFile() != null) r.add(PARENT_DIR);
            FilenameFilter filter = new FilenameFilter() {
                public boolean accept(File dir, String filename) {
                    File sel = new File(dir, filename);
                    if (!sel.canRead()) return false;
                    else {
                        boolean endsWith = mFileEnd == null || filename.toLowerCase().contains(mFileEnd);
                        return endsWith || sel.isDirectory();
                    }
                }
            };
            String[] fileList1 = path.list(filter);
            Collections.addAll(r, fileList1);
        }
        mFileList = r.toArray(new String[]{});
    }

    private File getChosenFile(String fileChosen) {
        if (fileChosen.equals(PARENT_DIR)) return mCurrentPath.getParentFile();
        else return new File(mCurrentPath, fileChosen);
    }

    private void setFileEndsWith(String fileEndsWith) {
        this.mFileEnd = fileEndsWith != null ? fileEndsWith.toLowerCase() : null;
    }
}

class ListenerList<L> {
    private List<L> listenerList = new ArrayList<>();

    public interface FireHandler<L> {
        void fireEvent(L listener);
    }

    public void add(L listener) {
        listenerList.add(listener);
    }

    public void fireEvent(FireHandler<L> fireHandler) {
        List<L> copy = new ArrayList<>(listenerList);
        for (L l : copy) {
            fireHandler.fireEvent(l);
        }
    }
}