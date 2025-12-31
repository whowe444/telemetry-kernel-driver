use posixmq::{OpenOptions, PosixMq};
use std::time::Duration;
use std::io;
use std::env;

fn main() -> io::Result<()> {
    // Get the queue name from command line arguments
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        eprintln!("Usage: {} <queue_name>", args[0]);
        return Ok(());
    }

    let queue_name = &args[1]; // The second argument is the queue name
    
    // Open or create a message queue.
    let mq : PosixMq = OpenOptions::readonly()
        .capacity(10)
        .max_msg_len(1024)
        .create()
        .open(queue_name)
        .expect("opening failed!");
    
    println!("Message Queue opened or created: {}", queue_name);

    // Read messages from the queue.
    loop {
        let mut buf = vec![0; mq.attributes().unwrap().max_msg_len];
        match mq.recv(&mut buf) {
            Ok((_priority, len)) => {
                println!("Received message: {:?}", &buf[..len]);
            },
            Err(e) => {
                eprintln!("Error receiving message: {}", e);
            }
        }
        
        // Optional: You can add a timeout or a sleep to avoid busy-waiting.
        std::thread::sleep(Duration::from_secs(1));
    }
}
