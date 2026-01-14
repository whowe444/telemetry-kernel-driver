use lapin::{BasicProperties, options::*, types::FieldTable, Connection, ConnectionProperties, Channel};
use posixmq::{OpenOptions, PosixMq};
use std::env;
use tokio;

async fn create_rabbit_queue(queue_name: &str) -> Result<Channel, Box<dyn std::error::Error>> {
    // Establish connection to RabbitMQ
    let conn = Connection::connect("amqp://guest:guest@localhost:5672", ConnectionProperties::default())
        .await?;
    let channel = conn.create_channel().await?;

    // Declare a queue
    let _rabbit_mq = channel
        .queue_declare(
            queue_name,           // Queue name
            QueueDeclareOptions::default(),
            FieldTable::default(),
        )
        .await?;
    return Ok(channel);
}

fn create_posix_queue(queue_name: &str) -> PosixMq {
    // Open or create a message queue.
    let mq : PosixMq = OpenOptions::readonly()
        .capacity(10)
        .max_msg_len(1024)
        .create()
        .open(queue_name)
        .expect("opening failed!");
    
    println!("Message Queue opened or created: {}", queue_name);
    mq
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Get the queue name from command line arguments
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        eprintln!("Usage: {} <queue_name>", args[0]);
        return Ok(());
    }

    let queue_name = &args[1]; // The second argument is the queue name

    // Create PosixMq
    let mq = create_posix_queue(queue_name);

    // Create RabbitMQ queue
    let channel = match create_rabbit_queue(queue_name).await {
        Ok(channel) => {
            println!("Channel created.");
            channel
        }
        Err(e) => {
            eprintln!("Error: {}", e);
            std::process::exit(1)
        }
    };

    // Read messages from the queue.
    loop {
        let mut buf = vec![0; mq.attributes().unwrap().max_msg_len];
        match mq.recv(&mut buf) {
            Ok((_priority, len)) => {
                println!("Received message: {:?}", String::from_utf8_lossy(&buf[..len]));
                channel
                    .basic_publish(
                        "",                      // Default exchange (empty means "amq.direct")
                        queue_name,              // Routing key is the queue name
                        BasicPublishOptions::default(),
                        &buf,      // Message body
                        BasicProperties::default(),
                    )
                    .await?
                    .await?;

                println!("Message sent to RabbitMQ!");
            },
            Err(e) => {
                eprintln!("Error receiving message: {}", e);
            }
        }
    }
}
